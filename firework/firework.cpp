#include "stdafx.h"

#define FIREWORK_SYSTEM_IMPL
#include <KR3/wl/eventhandle.h>
#include <KRMessage/promise.h>
#include <KRMessage/pump.h>
#include "mplayer.h"
#include "vote.h"
#include "chatpusher.h"

#include "firework.h"

#include "gapi.h"
#include "chat.h"
#include "fireluncher.h"
#include "msgobj.h"
#include "config.h"
#include "res.h"
#include "twitch.h"
#include "util.h"

using namespace kr;

namespace
{
	void showErrorText(AText16 message) noexcept
	{
		EventPump::getInstance()->postL(5_s, [message = move(message)](void*) {
			g_newdraws.attach(_new NotifyMessage({ 1,0,0,1 }, message));
		});
	}
}

FireworkSystem* FireworkSystem::create(double startTime) noexcept
{
	g_dc.now = startTime;
	return _new FireworkSystem;
}

FireworkSystem::FireworkSystem() noexcept
	:m_vote(300.f)
{
	m_chatPusher = _new ChattingPusher;
	m_started = false;
	m_resourceReady = false;
	m_connected = false;

	m_targetTexture = nullptr;
	m_promMgr = nullptr;
	m_stackAllocator = nullptr;
	m_pump = nullptr;
	m_canceler = nullptr;

	g_config.load();
}
FireworkSystem::~FireworkSystem() noexcept
{
	m_player.stop();

	destroy();
	DrawObject::clear();

	m_chatPusher->unload();
	m_chatPusher->Release();
	if (m_stackAllocator)
	{
		m_stackAllocator->terminate();
		m_stackAllocator = nullptr;
	}
	if (m_promMgr)
	{
		m_promMgr->finish();
		m_promMgr = nullptr;
	}
	if (m_pump)
	{
		m_pump->terminate();
		m_pump = nullptr;
	}
}

void FireworkSystem::createWindowFrom(ID3D11Texture2D * texture, int textureWidth, int textureHeight) noexcept
{
	if (m_targetTexture)
	{
		if (texture != m_targetTexture)
		{
			if (m_targetTexture)
			{
				((IUnknown*)m_targetTexture)->Release();
				m_targetTexture = nullptr;
			}
			m_targetTexture = texture;
			if (g_dc.isCreated())
			{
				_destroyDevice();
				_createDevice(textureWidth, textureHeight);
			}
		}
		return;
	}

	m_pump = EventPump::getInstance();
	m_promMgr = PromiseManager::getInstance();
	m_stackAllocator = StackAllocator::getInstance();
	
	_assert(textureWidth != 0 && textureHeight != 0);

	m_targetTexture = texture;
	((IUnknown*)m_targetTexture)->AddRef();

	_createDevice(textureWidth, textureHeight);
}
void FireworkSystem::destroy() noexcept
{
	m_player.stop();
	if (m_targetTexture)
	{
		((IUnknown*)m_targetTexture)->Release();
		m_targetTexture = nullptr;
	}
	if (m_canceler)
	{
		m_canceler->set();
		while (m_canceler != nullptr)
		{
			m_promMgr->process();
			Sleep(1);
		}
	}
	_destroyDevice();
}

void FireworkSystem::update() noexcept
{
	try
	{
		if (m_pump != nullptr)
		{
			m_pump->processOnce({ m_chatPusher->makeProcedure() });
		}
	}
	catch (QuitException&)
	{
	}
}
void FireworkSystem::render(double time) noexcept
{
	if (!g_dc.isCreated()) return;
	g_dc.delta = (float)(time - g_dc.now);
	g_dc.now = time;

	if (m_started)
	{
		_update();
	}
	else
	{
		g_dc->beginDraw();
		g_dc->clear({ 255, 255, 255, 0 });
		g_dc->fillRect({ 0.f, 0.f, (float)g_dc.width, (float)g_dc.height }, g_res->chatBackBrush);
		g_dc->strokeRect({ 10.f, 10.f, g_dc.width - 10.f, g_dc.height - 10.f }, g_res->red, 10);
		g_dc->endDraw();
		g_dc.present();
	}
}
void FireworkSystem::mouseDown() noexcept
{
	g_dc.mousePress = true;
	g_dc.mouseDown = true;
}
void FireworkSystem::mouseUp() noexcept
{
	g_dc.mouseDown = false;
	g_dc.mouseRelease = true;
}
void FireworkSystem::mouseMove(ivec2 pos) noexcept
{
	g_dc.mouse = (vec2)pos;
	g_dc.mouseIn = true;
}
void FireworkSystem::mouseOut() noexcept
{
	g_dc.mouseIn = false;
}

void FireworkSystem::_createDevice(int textureWidth, int textureHeight) noexcept
{
	if (!g_dc.create(m_targetTexture, textureWidth, textureHeight)) return;
	g_mix.create();
	_init();
}
void FireworkSystem::_destroyDevice() noexcept
{
	CachedImage::unloadAll();
	if (m_resourceReady)
	{
		m_resourceReady = false;
		g_res.remove();
	}
	g_dc.remove();
	g_mix.remove();
}
void FireworkSystem::_init() noexcept
{
	if (!m_resourceReady)
	{
		m_resourceReady = true;
		g_res.create();
	}
	_connect();
}
void FireworkSystem::_connect() noexcept
{
	m_chatPusher->load();

	if (m_connected) return;
	m_connected = true;
	if (m_canceler) return;
	m_canceler = EventHandle::create(false, true);

	twitch::loadBadges()->then([this]()->Promise<void>* {
		// auth
		if (g_config.testMode)
		{
			return Promise<void>::resolve();
		}
		else
		{
			return Promise<void>::resolve()->then([this] {
				if (g_config.enableTwitch)
				{
					return twitch::auth(m_canceler, "openid chat_login channel_read")->then([]() {
						return twitch::getChannel();
					})->then([this](twitch::Channel & info) {
						m_chatPusher->connectTwitchIrc(info);
					})->katch([this](exception_ptr ex) {
						showErrorText(u"twitch channel failed");
					});
				}
				return Promise<void>::resolve();
			})->then([this] {
				if (g_config.enableYoutube)
				{
					return gapi::auth(m_canceler);
				}
				return Promise<void>::resolve();
			});
		}
	})->then([this] {
		m_started = true;
		luncher::init();
		// load music
		if (!g_config.bgmPath.empty())
		{
			m_player.load(g_config.bgmPath);
		}
		
		// test message
		for (TestMessage & msg : g_config.testMessages)
		{
			for (uint i = 0; i < msg.count; i++)
			{
				if (msg.tier)
				{
					m_chatPusher->donate({ ServerType::Twitch, nullptr }, msg.icon, msg.name, msg.message, msg.tier, msg.amount, nullptr, nullptr);
				}
				else
				{
					m_chatPusher->chat({ ServerType::Twitch, nullptr }, msg.icon, msg.name, msg.message, nullptr, nullptr);
				}
			}
		}
		g_config.testMessages = nullptr;

		// read live chat
		if (g_config.enableYoutube)
		{
			gapi::youtube::liveBroadcast::list("snippet", gapi::youtube::liveBroadcast::BroadcastType::Persistent, true)
				->then([this](gapi::youtube::liveBroadcast::List& list) {
				if (list.items.empty())
				{
					AText errorMessage;
					if (list.error != nullptr) errorMessage = move(list.error->message);
					showErrorText(AText16() << u"Video Not Found\n" << utf8ToUtf16(errorMessage));
					return;
				}
				auto & video = list.items[0];
				m_chatPusher->setYouTubeVideoId(move(video.id));
				m_chatPusher->setYouTubeLiveChatId(move(list.items[0].snippet.liveChatId));
			})->katch([](Exception &) {
				showErrorText(u"live list failed");
			});
		}
		
		m_canceler = nullptr;
	})->katch([this](exception_ptr&){
		m_canceler = nullptr;
	});
}
void FireworkSystem::_update() noexcept
{
	// draw begin
	g_dc->beginDraw();
	g_dc->clear({ 0, 0, 0, 0 });
	
	// m_vote.draw();
	
	// draw
	luncher::drawGradView();

	m_chatPusher->drawAll();
	DrawObject::drawAll();
	m_player.draw();

	{
		d2d::Path path;
		path.create();
		{
			d2d::PathMake make = path.open();

			uint youtube = m_chatPusher->getYoutubeViewers();
			uint twitch = m_chatPusher->getTwitchViewers();
			if (youtube != -1 && twitch != -1)
			{
				TSZ16 text;
				if (youtube != -1)
				{
					text << youtube;
				}
				if (twitch != -1)
				{
					if (youtube != -1) text << u'/';
					text << twitch;
				}
				make.text(text, g_res->font);
			}
			make.close();
		}
		g_dc->setTransform(mat2p::translate(8, 55));
		g_dc->stroke(path, g_res->black, 5.f);
		g_dc->fill(path, g_res->white);
		g_dc->setTransform(mat2p::identity());
	}
	// g_dc->fillText();

	g_dc->endDraw();
	g_dc.present();

	// stepping process
	CachedImage::loadImage();

	m_chatPusher->push();

	// clear state
	g_dc.mousePress = false;
	g_dc.mouseRelease = false;
}

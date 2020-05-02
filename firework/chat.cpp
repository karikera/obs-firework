#include "stdafx.h"
#include "chat.h"
#include "drawcontext.h"
#include "res.h"
#include "config.h"

#include <KRUtil/fetch.h>
#include <KRUtil/fs/file.h>

#include "gapi.h"


namespace
{
	vec2 s_dragRelative;
	ChatCommon * s_dragging;
	const float DRAGGING_DURATION = 1.f;
	float s_chatYOffset;
	ChatUser s_recentlyBanned;
	bool s_removeBannedAll;
}

struct ExchangeRate
{
	Text16 symbol;
	float rate;
};

struct TierInfo
{
	uint cost;
	float widthRate;
	float heightRate;
	size_t textLength;
	float duration;
	float textDuration;
};

constexpr float NAME_WIDTH_RATE = 270.f / 21.f;
constexpr float NAME_HEIGHT_RATE = 28.f / 21.f;
constexpr float NAME_SPACE = 20.f;

const TierInfo TIER_INFOS[] = {
	{
		0,
		NAME_WIDTH_RATE, NAME_HEIGHT_RATE,
		0,
		5.f,
		0.f,
	},
	{
		2000,
		NAME_WIDTH_RATE / 270.f * 400.f, NAME_HEIGHT_RATE / 28.f * 120.f,
		50,
		10.f,
		20.f,
	},
	{
		5000,
		NAME_WIDTH_RATE / 270.f * 520.f, NAME_HEIGHT_RATE / 28.f * 160.f,
		150,
		2 * 60.f,
		30.f,
	},
	{
		10000,
		NAME_WIDTH_RATE / 270.f * 520.f, NAME_HEIGHT_RATE / 28.f * 180.f,
		200,
		5 * 60.f,
		40.f,
	},
	{
		20000,
		NAME_WIDTH_RATE / 270.f * 520.f, NAME_HEIGHT_RATE / 28.f * 200.f,
		225,
		10 * 60.f,
		45.f,
	},
	{
		50000,
		NAME_WIDTH_RATE / 270.f * 520.f, NAME_HEIGHT_RATE / 28.f * 220.f,
		250,
		30 * 60.f,
		50.f,
	},
	{
		100000,
		NAME_WIDTH_RATE / 270.f * 560.f, NAME_HEIGHT_RATE / 28.f * 220.f,
		270,
		60 * 60.f,
		54.f,
	},
	{
		200000,
		NAME_WIDTH_RATE / 270.f * 500.f, NAME_HEIGHT_RATE / 28.f * 220.f,
		290,
		120 * 60.f,
		58.f,
	},
	{
		300000,
		NAME_WIDTH_RATE / 270.f * 540.f, NAME_HEIGHT_RATE / 28.f * 220.f,
		310,
		180 * 60.f,
		62.f,
	},
	{
		400000,
		NAME_WIDTH_RATE / 270.f * 570.f, NAME_HEIGHT_RATE / 28.f * 220.f,
		330,
		4 * 60 * 60.f,
		66.f,
	},
	{
		500000,
		NAME_WIDTH_RATE / 270.f * 600.f, NAME_HEIGHT_RATE / 28.f * 220.f,
		350,
		5 * 60 * 60.f,
		70.f,
	}
};

constexpr float DOLLAR = 1118.50f;
const ExchangeRate RATES[] = {
{ u"￦", 1.f },
{ u"$", DOLLAR },
{ u"€", 1254.39f },
{ u"₽", 17.30f },
{ u"¥", 9.7804f },
{ u"Bit", DOLLAR / 100.f },
{u"달xPrime", DOLLAR*5.99f },
{u"달x4.99$", DOLLAR*4.99f },
{u"달x9.99$", DOLLAR*9.99f },
{u"달x24.99$", DOLLAR*24.99f },
{u"달x???", DOLLAR*5.99f },
};

constexpr float CHAT_PADDING_RATE = 6.f / 12.f;
constexpr float CHAT_ICON_PADDING_RATE = 4.f / 12.f;
constexpr float CHAT_NAME_PADDING_RATE = 8.f / 12.f;
constexpr float ICON_SIZE_RATE = 16.f / 12.f;

inline int calculateTier(uint won) noexcept
{
	int tier = -1;
	for (auto &info : TIER_INFOS)
	{
		if (won < info.cost)
		{
			return tier;
		}
		tier++;
	}
	return (int)(countof(TIER_INFOS) - 1);
}

ChatCommon::ChatCommon(BanListener * onban, ChatUser user, AText imageUrl, AText16 name,
	Text message, d2d::Font font, float boxWidth, 
	size_t loadingCount, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept
	:m_onban(onban),
	m_user(move(user)),
	m_image(imageUrl),
	m_badgeWidth(0),
	m_name(utf8ToUtf16(name)), 
	m_msgbox(message, move(font), boxWidth, emotes),
	m_badges(move(badges))
{
	AddRef();
	AddRef();

	m_lifeTime = 0;
	m_from = { 0, 0 };
	m_to = { 0, 0 };
	m_drawPos = { 0, 0 };
	m_startSpeed = { 0, 0 };
	m_startTime = g_dc.now;
	dead = false;
	m_floating = false;

	m_loading = loadingCount + 1;
	if (emotes != nullptr) m_loading += emotes.size();
	m_loading += m_badges.size();
	
	m_nameHeight = g_res->normalNameFont.getHeight();
}
ChatCommon::~ChatCommon() noexcept
{
}
void ChatCommon::startLoad() noexcept
{
	m_image.onLoad([this](bool) { _loadOnce();  });
	for (const TextBox::ImageCharacter& emote : m_msgbox.emotes())
	{
		emote.image->onLoad([this](bool) { _loadOnce(); });
	}
	for (ImageKeeper & img : m_badges)
	{
		img.onLoad([this](bool) { _loadOnce(); });
	}
	_loadOnce();
}
bool ChatCommon::update(float x, float y, float width, float height) noexcept
{
	if (s_dragging != this)
	{
		if (m_floating)
		{
			vec2 to = { x, y };

			// 3차 곡선을 그리며 움직이는 궤도 계산
			// x: 시간
			// y: 위치
			// f: 끝 위치
			vec2 f = to - m_from;
			// d: 시작 기울기
			vec2 d = m_startSpeed;

			// a,b: 미지수

			// a*x^3 +b*x^2 +d*x = y
			// 3*a*x^2 +2*b*x +d = y'

			// a*1^3 +b*1^2 +d*1 = f
			// a +b +d = f
			// a +b +d -f = 0

			// 3*a*1^2 +2*b*1 +d = 0
			// 3*a +2*b +d = 0

			// a +b +d -f = 0
			// a = -b -d +f
			// 3*a +2*b +d = 0
			// 3*(-b -d +f) +2*b +d = 0
			// -3*b -3*d +3*f +2*b +d = 0
			// -b -2*d +3*f = 0
			// b = 3*f -2*d
			vec2 b = 3.f * f - 2.f* d;

			// a = -b -d +f
			// a = -(3*f -2*d) -d +f
			// a = -3*f +2*d -d +f
			// a = -2*f +d
			// a = d -2*f
			vec2 a = d - 2.f * f;

			float t = (float)(g_dc.now - m_startTime);
			if (t >= 1.f)
			{
				m_floating = false;
				m_drawPos = m_to;

				if (m_to != to)
				{
					// 3*x^2*a + 2*x*b + c = y' // 기울기
					m_startSpeed = { 0.f, 0.f };
					m_from = m_drawPos;
					m_to = to;
					m_startTime = g_dc.now;
				}
			}
			else
			{
				float t3 = t * t * t;
				float t2 = t * t;

				// x^3*a + x^2*b + x*c = y // 그래프 방정식
				m_drawPos = t3 * a + t2 * b + t * d + m_from;

				if (m_drawPos.x > g_dc.width)
				{
					ban();
				}

				if (m_to != to)
				{
					// 3*x^2*a + 2*x*b + c = y' // 기울기
					m_startSpeed = 3 * t2 * a + 2 * t * b + d;
					m_from = m_drawPos;
					m_to = to;
					m_startTime = g_dc.now;
				}
			}
		} 
		else
		{
			m_drawPos.x = x;
			m_drawPos.y = y;
		}
	}

	frect chatRect(0, 0, width, height);

	bool hover = g_dc.mouseIn && chatRect.contains(g_dc.mouse - m_drawPos);
	g_dc->setTransform({1,0,0,1,m_drawPos.x, m_drawPos.y});
	g_dc->pushRectClip(chatRect);

	if (s_dragging == this)
	{
		return true;
	}
	if (hover && g_dc.mousePress && !s_dragging)
	{
		s_dragging = this;
		s_dragRelative = m_drawPos - g_dc.mouse;
	}

	return hover;
}
float ChatCommon::drawIcon(float x, float y, float alpha) noexcept
{
	float iconSize = g_config.normalChatFontSize * ICON_SIZE_RATE;
	switch (m_user.server)
	{
		case ServerType::Twitch: {
			if (alpha != 1.f) g_dc->pushLayer(g_res->layer, alpha);

			vec2 pos = { x, y };
			vec2 size = g_res->twitchImage.getSize();
			g_dc->drawImage(g_res->twitchImage, { pos, pos + size });
			pos.x += iconSize;

			for (auto & badge : m_badges)
			{
				vec2 size = badge.getSize();
				pos.y = y + (iconSize - size.y) * 0.5f;
				badge.draw(g_dc, {pos, pos + size});
				pos.x += size.x;
			}

			if (alpha != 1.f) g_dc->popLayer();
			break;
		}
		case ServerType::YouTube: {
			if (!m_image.isLoaded()) break;
			float R = iconSize * 0.5f;

			d2d::Path path;
			{
				path.create();
				d2d::PathMake make = path.open();

				make.begin({ x, y + R });
				make.arcTo({ x + R * 2, y + R }, { R, R });
				make.arcTo({ x, y + R }, { R, R });
				make.end(true);
				make.close();
			}

			g_dc->pushLayer(g_res->layer, path, math::mat2p::identity(), alpha);
			vec2 size = m_image.getSize();
			mat2p old = g_dc->getTransform();
			g_dc->setTransform(mat2p(old).postTranslate({ x, y }).preScale(iconSize / size));
			m_image.draw(g_dc);
			g_dc->setTransform(old);
			g_dc->popLayer();
			break;
		}
	}
	return x + m_badgeWidth + iconSize + CHAT_ICON_PADDING_RATE * g_config.normalChatFontSize;
}
void ChatCommon::ban() noexcept
{
	s_recentlyBanned = m_user;
	s_removeBannedAll = true;
	m_onban->onBan(this);
}
ServerType ChatCommon::getServer() noexcept
{
	return m_user.server;
}
Text ChatCommon::getId() noexcept
{
	return m_user.id;
}
void ChatCommon::onLoadEnd() noexcept
{
}
void ChatCommon::_loadOnce() noexcept
{
	m_loading--;
	if (m_loading == 0)
	{
		onLoad.fire();
		onLoad.clear();
		m_image.resetFrameTime();
		float badgeWidth = 0;
		for (auto & badge : m_badges)
		{
			badgeWidth += badge.getSize().x;
		}
		m_badgeWidth = badgeWidth;

		onLoadEnd();
		Release();
	}
}

NormalChat::NormalChat(BanListener * onban, ChatUser user, AText imageUrl, AText16 name, Text message, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept
	:ChatCommon(onban, move(user), imageUrl, move(name),
		message, g_res->normalMessageFont, 
		g_config.normalChatWidth - g_config.normalChatFontSize * (CHAT_PADDING_RATE * 2.f),
		1, emotes, move(badges))
{
	m_floating = true;
	m_to = m_from = { (float)g_dc.width - g_config.normalChatWidth, (float)g_dc.height };
	startLoad();
}
NormalChat::~NormalChat() noexcept
{
}
float NormalChat::draw(float y) noexcept
{
	float x = g_dc.width - g_config.normalChatWidth;
	float height = m_msgbox.getHeight() + g_config.normalChatFontSize * (CHAT_PADDING_RATE * 2.f);
	float resy = y - height;
	if (resy >= g_dc.height) return resy;


	if (!g_dc.mouseIn) m_lifeTime += g_dc.delta;
	if (m_lifeTime > g_config.chatDuration + 1)
	{
		dead = true;
		return y;
	}

	bool hover = update(x, resy, g_config.normalChatWidth, height);

	d2d::Brush chatBackBrush;
	float alpha = 1.f;
	if (hover)
	{
		chatBackBrush = g_res->white;
	}
	else if (m_lifeTime > g_config.chatDuration)
	{
		alpha = 1 - (m_lifeTime - g_config.chatDuration);
		chatBackBrush = g_dc->createSolidBrush({ 1.f,1.f,1.f,alpha * 0.5f });
	}
	else
	{
		chatBackBrush = g_res->chatBackBrush;
	}

	frect chatRect(0, 0, g_config.normalChatWidth, height);
	chatRect.left++;
	chatRect.top++;
	chatRect.right--;
	chatRect.bottom--;
	g_dc->fillRect(chatRect, chatBackBrush);

	if (m_msgbox.isCalculated())
	{
		float texty = (height - m_msgbox.getHeight()) / 2;
		float icony = texty + (m_msgbox.getLineHeight(0) - g_config.normalChatFontSize * ICON_SIZE_RATE) / 2;

		float nextX = drawIcon(g_config.normalChatFontSize * CHAT_PADDING_RATE, icony, alpha);

		if (alpha != 1.f) g_dc->pushLayer(g_res->layer, alpha);
		g_dc->fillText(m_name, g_res->normalNameFont, { nextX, texty }, g_res->normalChatNameBrush);
		m_msgbox.draw(g_config.normalChatFontSize * CHAT_PADDING_RATE, texty, g_res->black);
		if (alpha != 1.f) g_dc->popLayer();
	}
	g_dc->popRectClip();

	if (hover) g_dc->strokeRect(chatRect, g_res->capturePen, 2);
	g_dc->setTransform(mat2p::identity());

	return resy;
}
void NormalChat::onLoadEnd() noexcept
{
	float indentation = g_dc->measureText(m_name, g_res->normalNameFont) + g_config.normalChatFontSize * (ICON_SIZE_RATE + CHAT_ICON_PADDING_RATE + CHAT_NAME_PADDING_RATE) + m_badgeWidth;
	m_msgbox.calculateTextArea(indentation);
	s_chatYOffset += m_msgbox.getHeight();
}

SuperChat::SuperChat(BanListener* onban, ChatUser user, AText imageUrl, AText16 name, Text message, int tier, Text cost, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept
	:ChatCommon(onban, move(user), imageUrl, move(name),
		message, g_res->superMessageFont, _setTierAndReturnWidth(tier, cost),
		2, emotes, move(badges)),
	m_cost(utf8ToUtf16(cost))
{
	m_ttsStarted = false;

	m_won = 0;
	for (const ExchangeRate & rate : RATES)
	{
		if (m_cost.endsWith(rate.symbol))
		{
			m_won = (uint)((m_cost.cut(m_cost.size() - 1).to_float()) * rate.rate);
			break;
		}
	}

	const TierInfo & tierinfo = TIER_INFOS[m_tier];

	m_costWidth = g_dc->measureText(m_cost, g_res->superNameFont);

	if (!message.empty())
	{
		//MCISound * snd = new MCISound;
		//gapi::tts(message)->then([this, snd](AText16 & filename) {
		//	m_ttsFilename = move(filename);
		//	return snd->open(m_ttsFilename);
		//})->then([this, snd]() {
		//	m_ttsDuration = snd->duration() / 1000.f;

		//	AddRef();
		//	_loadOnce();

		//	EventPump::getInstance()->postL(1_s, [this, snd](void*) {
		//		m_ttsStartTime = g_now;
		//		m_ttsStarted = true;
		//		snd->playAnd()->then([this, snd]() {
		//			delete snd;
		//			File::remove(m_ttsFilename.data());
		//			m_ttsFilename = nullptr;
		//			Release();
		//		});
		//	});
		//})->katch([this, snd](Exception &) {
		//	_loadOnce();
		//	delete snd;
		//});
	}
	else
	{
		_loadOnce();
	}
	startLoad();
}
SuperChat::~SuperChat() noexcept
{
	if (!m_ttsFilename.empty())
	{
		File::remove(m_ttsFilename.data());
	}
}

uint SuperChat::getWon() noexcept
{
	return m_won;
}
float SuperChat::draw(float y) noexcept
{
	const TierInfo & tier = TIER_INFOS[m_tier];
	float width = tier.widthRate * g_config.superChatFontSize;
	float height = tier.heightRate * g_config.superChatFontSize;
	constexpr float margin = 10;
	constexpr float radius = 10;
	
	float nextGap = height + margin;

	if (m_loading != 0) return y - nextGap;
	if (!m_msgbox.isCalculated()) return y - nextGap;

	float textY = NAME_SPACE + (height - NAME_SPACE - m_msgbox.getHeight()) * 0.5f;

	if (!g_dc.mouseIn) m_lifeTime += g_dc.delta;
	if (m_lifeTime > tier.duration + 1)
	{
		dead = true;
		return y;
	}
	else if (m_lifeTime > tier.duration)
	{
		float t = 1 - (m_lifeTime - tier.duration);
		nextGap *= t;
		height *= t;
	}

	nextGap -= height;
	bool drawText = true;
	float nameWidth = g_config.superChatFontSize * NAME_WIDTH_RATE;
	float nameHeight = g_config.superChatFontSize * NAME_HEIGHT_RATE;
	if (height > nameHeight)
	{
		if (m_lifeTime > tier.textDuration + 1)
		{
			width = nameWidth;
			height = nameHeight;
			drawText = false;
		}
		else if (m_lifeTime > tier.textDuration)
		{
			float t = 1 - (m_lifeTime - tier.textDuration);
			width = (width - nameWidth) * t + nameWidth;
			height = (height - nameHeight) * t + nameHeight;
		}
	}

	nextGap += height;
	y -= nextGap;

	uint tierColorIndex = mint((uint)m_tier, (uint)countof(g_res->tiers) - 1);
	if (m_lifeTime < 1.f)
	{
		float v = 1.f - m_lifeTime;
		y -= v * v * 100.f;
	}

	bool hover = update(margin, y, width, height);

	frect chatRect(0, 0, width, height);
	g_dc->fillRect(chatRect, hover ? g_res->tiersHover[tierColorIndex] : g_res->tiers[tierColorIndex]);
	
	float textX = radius;
	float nameX = drawIcon(textX, 2.f);
	g_dc->fillText(m_name, g_res->superNameFont, 
		{ nameX, 4.f }, g_res->white);
	g_dc->fillText(m_cost, g_res->superNameFont,
		{ width - radius - m_costWidth, 4.f }, g_res->white);
	g_dc->popRectClip();
	
	if (drawText)
	{
		float clipAxisX;
		if (m_ttsStarted)
		{
			float current = (float)(g_dc.now - m_ttsStartTime) / m_ttsDuration;
			clipAxisX = current * m_msgbox.getWidth();
		}
		else
		{
			clipAxisX = 0.f;
		}
		clipAxisX += textX;

		g_dc->pushRectClip({ textX, 0, clipAxisX, height });
		m_msgbox.draw(textX, textY, g_res->white);
		g_dc->popRectClip();

		g_dc->pushRectClip({ clipAxisX, 0, textX + m_msgbox.getWidth(), height });
		m_msgbox.draw(textX, textY, g_res->gray);
		g_dc->popRectClip();
	}

	g_dc->setTransform(mat2p::identity());

	return y;
}
void SuperChat::onLoadEnd() noexcept
{
	m_msgbox.calculateTextArea(0);
}

float SuperChat::_setTierAndReturnWidth(int tier, Text cost) noexcept
{
	tier--;
	if ((uint)tier < (uint)countof(TIER_INFOS) - 1)
	{
		m_tier = tier;
	}
	else
	{
		m_tier = calculateTier(m_won);
	}
	return TIER_INFOS[m_tier].widthRate * g_config.superChatFontSize;
}

ChatList::ChatList(BanListener * onban) noexcept
	:m_onban(onban)
{
}
ChatList::~ChatList() noexcept
{
	clear();
}
void ChatList::clear() noexcept
{
	m_donations.forEachForRemove([this](SuperChat* node) {
		m_donations.detach(node);
		node->Release();
	});
	m_chattings.clear();
}
void ChatList::drawAll() noexcept
{
	float y = g_dc.height - GRAD_HEIGHT_BASIC - GRAD_HEIGHT_EXT;
	for (SuperChat & dona : m_donations)
	{
		y = dona.draw(y);
	}

	if (!g_dc.mouseIn) s_chatYOffset *= powf(0.005f, g_dc.delta);
	y = (float)g_dc.height + s_chatYOffset - g_config.characterSpace;
	for (NormalChat & chat : m_chattings)
	{
		if (y <= 0.f)
		{
			if (s_dragging == &chat) continue;
			chat.dead = true;
			continue;
		}
		y = chat.draw(y);
	}

	// kill banned
	if (s_removeBannedAll)
	{
		s_removeBannedAll = false;
		float setFloatingAfter = false;
		for (SuperChat & chat : m_donations)
		{
			if (chat.m_user == s_recentlyBanned)
			{
				chat.dead = true;
				setFloatingAfter = true;
			}
			if (setFloatingAfter)
			{
				chat.m_floating = true;
			}
		}
		for (NormalChat & chat : m_chattings)
		{
			if (chat.m_user == s_recentlyBanned)
			{
				chat.dead = true;
				setFloatingAfter = true;
			}
			if (setFloatingAfter)
			{
				chat.m_floating = true;
			}
		}
	}

	// remove dead
	{
		m_donations.forEachForRemove([this](SuperChat* node) {
			if (node->dead)
			{
				m_donations.detach(node);
				node->Release();
			}
		});
		m_chattings.forEachForRemove([this](NormalChat* node) {
			if (node->dead)
			{
				m_chattings.detach(node);
				node->Release();
			}
		});
	}

	// release
	if (s_dragging)
	{
		vec2 npos = g_dc.mouse + s_dragRelative;
		s_dragging->m_startSpeed = (npos - s_dragging->m_drawPos) / g_dc.delta;
		s_dragging->m_startTime = g_dc.now;
		s_dragging->m_from = npos;
		s_dragging->m_to = npos;
		s_dragging->m_drawPos = npos;
		
		if (g_dc.mouseRelease)
		{
			s_dragging->m_floating = true;
			s_dragging = nullptr;
		}
	}

}
NormalChat* ChatList::makeNormalChat(ChatUser user, AText imageUrl, Text name, Text message, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept
{
	if (s_recentlyBanned == user)
	{
		return nullptr;
	}
	NormalChat * chat = reline_new(m_chattings.createFirst(
		m_onban, move(user), move(imageUrl), move(AText16() << utf8ToUtf16(name)), 
		message, emotes, move(badges)));
	s_chatYOffset += g_config.normalChatFontSize * (CHAT_PADDING_RATE * 2.f);
	return chat;
}
SuperChat * ChatList::makeDonation(ChatUser user, AText imageUrl, AText16 name, Text message, int tier, Text cost, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept
{
	SuperChat * chat = m_donations.create(m_onban, move(user), move(imageUrl), move(AText16() << utf8ToUtf16(name)),
		message, tier, cost, emotes, move(badges));
	return chat;
}
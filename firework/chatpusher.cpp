#include "stdafx.h"
#include "chatpusher.h"

#include "fireluncher.h"
#include "msgobj.h"
#include "config.h"
#include "twitch.h"
#include "mfmix.h"

ChattingPusher::ChattingPusher() noexcept
	:m_logFile(), m_donateLog(&m_logFile), m_chatList(this)
{
	m_youtubeViewers = -1;
	m_twitchViewers = -1;
	m_unloading = false;
	m_loaded = false;
	AddRef();
}
ChattingPusher::~ChattingPusher() noexcept
{
}
void ChattingPusher::load() noexcept
{
	if (m_loaded) return;
	m_loaded = true;
	m_logFile.create(u"donation.log");
	_requestViewers();
}
void ChattingPusher::unload() noexcept
{
	m_chatList.clear();
	m_chattingsQueue.clear();
	m_youtubeVideoId = nullptr;
	m_twitchChannelId = nullptr;
	m_unloading = true;
}
void ChattingPusher::donate(ChatUser user, AText imageUrl, Text name, Text message, int tier, Text cost, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept
{
	m_donateLog
		<< UnixTimeStamp::now().getInfo()
		<< " [" << tier << "] "
		<< name
		<< "(" << user.id << "): "
		<< message;
	m_donateLog.flush();

	SuperChat * dona = m_chatList.makeDonation(move(user), move(imageUrl), move(AText16() << utf8ToUtf16(name)), message, tier, cost, emotes, move(badges));
	AddRef();
	dona->onLoad.add([this, dona] {
		if (testRelease()) return;
		g_mix.play(g_config.donateSound.data());
		//uint won = dona->getWon();
		//luncher::chargeValue(won);
	});
}
void ChattingPusher::chat(ChatUser user, AText imageUrl, Text name, Text message, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept
{
	m_chatList.makeNormalChat(move(user), move(imageUrl), name, message, emotes, move(badges));
}
void ChattingPusher::setYouTubeLiveChatId(AText liveChatId) noexcept
{
	m_liveChatId = move(liveChatId);
	if (m_liveChatId == nullptr)
	{
		EventPump::getInstance()->postL(5_s, [this](void*) {
			g_newdraws.attach(_new NotifyMessage({ 1,0,0,1 }, AText16() << u"No Live Chat In Video: " << utf8ToUtf16(m_youtubeVideoId)));
		});
		return;
	}
	_requestYouTubeChattings(nullptr);
}
void ChattingPusher::setYouTubeVideoId(AText videoId) noexcept
{
	m_youtubeVideoId = move(videoId);
}
void ChattingPusher::push() noexcept
{
	if (m_unloading) return;
	if (!m_chattingsQueue.empty())
	{
		auto* item = m_chattingsQueue.detachFirst();
		auto& author = item->authorDetails;
		auto& superchat = item->snippet.superChatDetails;
		ChatUser user = { ServerType::YouTube, author.channelId };
		if (superchat.tier)
		{
			donate(
				move(user),
				author.profileImageUrl,
				author.displayName,
				superchat.userComment,
				superchat.tier,
				superchat.amountDisplayString,
				nullptr,
				nullptr);
		}
		else
		{
			chat(
				move(user),
				author.profileImageUrl,
				author.displayName,
				item->snippet.displayMessage,
				nullptr,
				nullptr);
		}
		delete item;
	}
}
void ChattingPusher::drawAll() noexcept
{
	m_chatList.drawAll();
}
void ChattingPusher::onBan(ChatCommon* chat) noexcept
{
	uint duration = 30 * 60;

	switch (chat->getServer())
	{
	case ServerType::YouTube:
		gapi::youtube::liveChatBan::insert(m_liveChatId, chat->getId(), duration);
		break;
	case ServerType::Twitch:
		ircSend(TSZ() << "@ban-duration=" << duration << " :tmi.twitch.tv CLEARCHAT #" << m_twitchChannelId << " :" << chat->getId() << "\r\n");
		break;
	}
}
uint ChattingPusher::getYoutubeViewers() noexcept
{
	return m_youtubeViewers;
}
uint ChattingPusher::getTwitchViewers() noexcept
{
	return m_twitchViewers;
}
bool ChattingPusher::testRelease() noexcept
{
	if (m_unloading)
	{
		Release();
		return true;
	}
	Release();
	return false;
}
Text ChattingPusher::getYouTubeLiveChatId() noexcept
{
	return m_liveChatId;
}
void ChattingPusher::connectTwitchIrc(twitch::Channel & info) noexcept
{
	if (!isConnected())
	{
		ircCap("twitch.tv/tags twitch.tv/commands twitch.tv/membership");
		ircPass(TSZ() << "oauth:" << twitch::getAccessToken());
		ircConnect(
			u"irc.twitch.tv", 6667,
			nullptr, nullptr,
			info.name);
		m_twitchChannelName = move(info.name);
		m_twitchChannelId = move(info._id);
	}
}
void ChattingPusher::_requestViewers() noexcept
{
	TmpArray<Promise<void>*> proms;

	if (m_youtubeVideoId != nullptr)
	{
		AddRef();
		proms.push(gapi::youtube::liveBroadcast::getViewers(m_youtubeVideoId)
			->then([this](uint count) {
			if (count != -1)
			{
				m_youtubeViewers = count;
			}
			Release();
		})->katch([this](Exception&) {
			Release();
		}));
	}
	if (m_twitchChannelId != nullptr)
	{
		AddRef();
		proms.push(twitch::getStreamByUser(m_twitchChannelId)
			->then([this](twitch::Stream& stream) {
			if (stream.stream.viewers != -1)
			{
				m_twitchViewers = stream.stream.viewers;
			}
			Release();
		})->katch([this](Exception&) {
			Release();
		}));
	}

	AddRef();
	Promise<void>::all(proms)->then([this] {
		if (m_unloading)
		{
			Release();
			throw ThrowAbort();
		}
		return EventPump::getInstance()->promise(5_s);
	})->then([this] {
		if (testRelease()) return;
		_requestViewers(); 
	});
}
void ChattingPusher::_requestYouTubeChattings(AText pageNextToken) noexcept
{
	AddRef();
	Text token = pageNextToken;
	gapi::youtube::liveChatMessage::list(m_liveChatId, "snippet,authorDetails", token)
		->then([this](gapi::youtube::liveChatMessage::List& chat) {
		if (m_unloading)
		{
			Release();
			return;
		}

		for (auto & item : chat.items)
		{
			m_chattingsQueue.create(move(item));
		}

		if (chat.pollingIntervalMillis == 0)
		{
			chat.pollingIntervalMillis = 2000;
		}
		duration wait = (duration)chat.pollingIntervalMillis;
		EventPump::getInstance()->postL(wait, [this, pageNextToken = move(chat.nextPageToken)](void*){
			if (testRelease()) return;
			_requestYouTubeChattings(move(pageNextToken));
		});
	})->katch([this, pageNextToken = move(pageNextToken)](Exception&) {
		if (m_unloading)
		{
			Release();
			return;
		}
		EventPump::getInstance()->postL(2_s, [this, pageNextToken = move(pageNextToken)](void*) {
			if (testRelease()) return;
			_requestYouTubeChattings(move(pageNextToken));
		});
	});
}

void ChattingPusher::onUserDeleted(irc::User * user) noexcept
{
	delete (TwitchUserInfoCommon*)user->param;
}
void ChattingPusher::onIRCConnect() noexcept
{
	TSZ channelName;
	channelName << "#" << m_twitchChannelName;
	ircJoin(channelName);
}
void ChattingPusher::onLine(Text line) noexcept
{
	udout << utf8ToUtf16(line) << endl;
}
void ChattingPusher::onNotice(int num, Text message) noexcept
{
	// dout << "> " << num << ' ' << message << endl;
}
void ChattingPusher::onMessage(irc::User * user, irc::Channel * channel, Text message) noexcept
{
	TwitchPrivMsg info;
	info.parse(tags);
	TwitchUserInfo * userinfo = info.moveUserInfoTo(user);

	ChatUser chatUser = { ServerType::Twitch, user->nick() };

	if (info.bits != 0)
	{
		donate(
			move(chatUser),
			nullptr,
			userinfo->displayName,
			message,
			0,
			TSZ() << info.bits << "Bit",
			move(info.emotes),
			move(info.badges));
	}
	else
	{
		chat(
			move(chatUser),
			nullptr,
			userinfo->displayName,
			message,
			move(info.emotes),
			move(info.badges));
	}
}
void ChattingPusher::onUnprocessed(irc::User * user, View<Text> messages) noexcept
{
	Text command = messages[0];

	if (command == "USERNOTICE")
	{
		Text channel = messages[1];
		Text message = messages[2];

		TwitchUserNotice notice;
		notice.parse(tags);
		irc::User * user = ircFindUser(notice.login);
		TwitchUserInfo * userinfo = notice.moveUserInfoTo(user);

		if (notice.msgId == TwitchUserNotice::MsgId::sub || notice.msgId == TwitchUserNotice::MsgId::resub)
		{
			Text postfix;
			if (notice.msgParamSubPlan == "Prime")
			{
				postfix = "含xPrime";
			}
			else if (notice.msgParamSubPlan == "1000")
			{
				postfix = "含x4.99$";
			}
			else if (notice.msgParamSubPlan == "2000")
			{
				postfix = "含x9.99$";
			}
			else if (notice.msgParamSubPlan == "3000")
			{
				postfix = "含x24.99$";
			}
			else
			{
				postfix = "含x???";
			}

			donate(
				{ ServerType::Twitch, user->nick() },
				nullptr,
				notice.displayName,
				message,
				0,
				TSZ() << notice.msgParamMonths << postfix,
				notice.emotes,
				move(notice.badges));
		}


		// resubscribe
		// @badges=staff/1,broadcaster/1,turbo/1;color=#008000;display-name=ronni;emotes=;id=db25007f-7a18-43eb-9379-80131e44d633;login=ronni;mod=0;msg-id=resub;msg-param-months=6;msg-param-sub-plan=Prime;msg-param-sub-plan-name=Prime;room-id=1337;subscriber=1;system-msg=ronni\shas\ssubscribed\sfor\s6\smonths!;tmi-sent-ts=1507246572675;turbo=1;user-id=1337;user-type=staff :tmi.twitch.tv USERNOTICE #dallas :Great stream -- keep it up!

		// gifted
		// @badges=staff/1,premium/1;color=#0000FF;display-name=TWW2;emotes=;id=e9176cd8-5e22-4684-ad40-ce53c2561c5e;login=tww2;mod=0;msg-id=subgift;msg-param-months=1;msg-param-recipient-display-name=Mr_Woodchuck;msg-param-recipient-id=89614178;msg-param-recipient-name=mr_woodchuck;msg-param-sub-plan-name=House\sof\sNyoro~n;msg-param-sub-plan=1000;room-id=19571752;subscriber=0;system-msg=TWW2\sgifted\sa\sTier\s1\ssub\sto\sMr_Woodchuck!;tmi-sent-ts=1521159445153;turbo=0;user-id=13405587;user-type=staff :tmi.twitch.tv USERNOTICE #forstycup

		// Notice sent when a channel is raided.
		// @badges=turbo/1;color=#9ACD32;display-name=TestChannel;emotes=;id=3d830f12-795c-447d-af3c-ea05e40fbddb;login=testchannel;mod=0;msg-id=raid;msg-param-displayName=TestChannel;msg-param-login=testchannel;msg-param-viewerCount=15;room-id=56379257;subscriber=0;system-msg=15\sraiders\sfrom\sTestChannel\shave\sjoined\n!;tmi-sent-ts=1507246572675;tmi-sent-ts=1507246572675;turbo=1;user-id=123456;user-type= :tmi.twitch.tv USERNOTICE #othertestchannel

		// Notice sent for a new_chatter ritual.
		// @badges=;color=;display-name=SevenTest1;emotes=30259:0-6;id=37feed0f-b9c7-4c3a-b475-21c6c6d21c3d;login=seventest1;mod=0;msg-id=ritual;msg-param-ritual-name=new_chatter;room-id=6316121;subscriber=0;system-msg=Seventoes\sis\snew\shere!;tmi-sent-ts=1508363903826;turbo=0;user-id=131260580;user-type= :tmi.twitch.tv USERNOTICE #seventoes :HeyGuys
	}
	else if (command == "USERSTATE")
	{
		TwitchUserState state;
		state.parse(tags);
		// @color=#0D4200;display-name=ronni;emote-sets=0,33,50,237,793,2126,3517,4578,5569,9400,10337,12239;mod=1;subscriber=1;turbo=1;user-type=staff :tmi.twitch.tv USERSTATE #dallas
	}

	// dout << join(messages, " ") << endl;
}

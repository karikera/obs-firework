#pragma once

#include <KRMessage/fs/asyncfile.h>
#include <KRMessage/net/IRC.h>

#include "chat.h"
#include "gapi.h"

using namespace kr;


class ChattingPusher:public Referencable<ChattingPusher>, public BanListener, private kr::irc::IRC
{
public:
	ChattingPusher() noexcept;
	~ChattingPusher() noexcept;

	void load() noexcept;
	void unload() noexcept;
	void donate(ChatUser user, AText imageUrl, Text name, Text message, int tier, Text cost, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept;
	void chat(ChatUser user, AText imageUrl, Text name, Text message, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept;
	void setYouTubeLiveChatId(AText liveChatId) noexcept;
	void setYouTubeVideoId(AText videoId) noexcept;
	void push() noexcept;
	void drawAll() noexcept;
	void onBan(ChatCommon* chat) noexcept override;
	uint getYoutubeViewers() noexcept;
	uint getTwitchViewers() noexcept;
	bool testRelease() noexcept;
	Text getYouTubeLiveChatId() noexcept;
	void connectTwitchIrc(twitch::Channel & info) noexcept;
	using kr::irc::IRC::makeProcedure;

private:
	void onUserDeleted(kr::irc::User * user) noexcept override;
	void onIRCConnect() noexcept override;
	void onLine(kr::Text line) noexcept override;
	void onNotice(int num, kr::Text message) noexcept override;
	void onMessage(kr::irc::User * user, kr::irc::Channel * channel, kr::Text message) noexcept override;
	void onUnprocessed(kr::irc::User * user, kr::View<kr::Text> messages) noexcept override;

private:
	AText m_liveChatId;
	AText m_twitchChannelId;
	AText m_youtubeVideoId;
	AText m_twitchChannelName;
	LinkedList<Node<gapi::youtube::liveChatMessage::Resource>> m_chattingsQueue;
//	MCISound m_chaching;
	ChatList m_chatList;
	AsyncFile m_logFile;
	io::BufferedOStream<AsyncFile::Stream<char>> m_donateLog;
	uint m_youtubeViewers;
	uint m_twitchViewers;
	bool m_unloading;
	bool m_loaded;

	void _requestYouTubeChattings(AText pageNextToken) noexcept;
	void _requestViewers() noexcept;

};
#pragma once

#include <KR3/util/callable.h>
#include <KR3/data/map.h>
#include <KR3/wl/eventhandle.h>
#include <KRUtil/parser/jsonparser.h>
#include <KRMessage/promise.h>
#include "kr$gapi$youtube.ajax.h"
#include "kr$twitch.ajax.h"

namespace kr
{
	namespace gapi
	{
		Promise<void> * auth(EventHandle * canceler) noexcept;
		Promise<void>* revoke() noexcept;
		Text getAccessToken() noexcept;
		Promise<AText>* getTokenInfo() noexcept;
		Promise<AText16>* download(const char * url, Text16 ext) noexcept;
		AText callSync(const char * url, AText data = nullptr) throws(HttpException);
		Promise<AText> * call(const char * url, AText data = nullptr) noexcept;
		Promise<AText> * callDelete(const char * url) noexcept;
		Promise<AText16>* tts(Text text) noexcept;

		namespace youtube
		{
			namespace liveChatBan
			{
				void remove(Text banId) noexcept;
				void insert(Text liveChatId, Text channelId, uint duration) noexcept;
			}

			namespace liveChatMessage
			{
				Promise<List> * list(Text liveChatId, Text part, Text pageToken) noexcept;
			}

			namespace liveBroadcast
			{
				enum class BroadcastType
				{
					All,
					Event,
					Persistent
				};
				Promise<List> * list(Text part, BroadcastType broadcastType = BroadcastType::All, bool mine = false) noexcept;
				Promise<uint>* getViewers(Text videoId) noexcept;
			}

			namespace video
			{
				Promise<List>* list(Text id, Text part) noexcept;
				void getLiveVideo(Text channelId) noexcept;
			}
		}
	}

	namespace twitch
	{
		AText callSync(const char * url, AText data = nullptr) throws (HttpException);

		Promise<void>* auth(EventHandle * canceler, AText scope) noexcept;
		Promise<void>* revoke() noexcept;
		Text getAccessToken() noexcept;

		Promise<void>* loadBadges() noexcept;
		Text getBadgeUrl(Text name, uint version) noexcept;
		Promise<Channel>* getChannel() noexcept;
		Promise<Channel>* getChannelById(AText channelId) noexcept;
		Promise<ChatRooms>* getChatRooms(AText channelId) noexcept;
		Promise<Stream>* getStreamByUser(AText channelId) noexcept;
	}
}

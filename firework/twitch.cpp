#include "stdafx.h"
#include "twitch.h"

#include "gapi.h"

template <typename Derived, typename Info>
void ircUnescape(OutStream<Derived, char, Info> *dest, Text src) noexcept
{
	for (;;)
	{
		Text readed = src.readwith('\\');
		if (readed == nullptr)
		{
			*dest << src;
			return;
		}
		*dest << readed;
		if (src.empty())
		{
			*dest << '\\';
			return;
		}
		char next = src.read();
		switch (next)
		{
		case 's': *dest << ' '; break;
		case '\\': *dest << '\\'; break;
		case ':': *dest << ':'; break;
		case 'r': *dest << '\r'; break;
		case 'n': *dest << '\n'; break;
		default:
			*dest << '\\';
			*dest << next;
			break;
		}
	}
}

TwitchUserInfo * TwitchUserInfo::get(irc::User * user) noexcept
{
	if (user->param) return (TwitchUserInfo*)user->param;
	TwitchUserInfo * info = _new TwitchUserInfo;
	user->param = info;
	return info;
}
void TwitchUserInfoCommon::parse(ReferenceMap<Text, Text>& tags) noexcept
{
	tags.ifGet("color", [&](Text value) {
		// #rrggbb
		if (value.size() == 7 && *value == '#')
		{
			dword r = value.read(2).to_uint(16);
			dword g = value.read(2).to_uint(16);
			dword b = value.to_uint(16);
			this->color = (r << 16) | (g << 8) | b;
		}
	});
	tags.ifGet("display-name", [&](Text value) {
		displayName.reserve(32);
		ircUnescape(&displayName, value);
	});
	tags.ifGet("mod", [&](Text value) {
		mod = value != "0";
	});
	tags.ifGet("subscriber", [&](Text value) {
		subscriber = value != "0";
	});
	tags.ifGet("turbo", [&](Text value) {
		turbo = value != "0";
	});
	tags.ifGet("user-type", [&](Text value) {
		if (value == "mod")
		{
			userType = UserType::mod;
		}
		else if (value == "global_mod")
		{
			userType = UserType::global_mod;
		}
		else if (value == "admin")
		{
			userType = UserType::admin;
		}
		else if (value == "staff")
		{
			userType = UserType::staff;
		}
	});
}
TwitchUserInfo * TwitchUserInfoCommon::moveUserInfoTo(irc::User * to) noexcept
{
	TwitchUserInfo * userinfo = TwitchUserInfo::get(to);
	TwitchUserInfoCommon * cast = userinfo;
	*cast = move(*this);
	return userinfo;
}

void TwitchUserNotice::parse(ReferenceMap<Text, Text>& tags) noexcept
{
	TwitchUserMessageLike::parse(tags);
	tags.ifGet("msg-id", [&](Text value) {
		if (value == "sub")
		{
			msgId = MsgId::sub;
		}
		else if (value == "resub")
		{
			msgId = MsgId::resub;
		}
		else if (value == "subgift")
		{
			msgId = MsgId::subgift;
		}
		else if (value == "raid")
		{
			msgId = MsgId::raid;
		}
		else if (value == "ritual")
		{
			msgId = MsgId::ritual;
		}
	});
	tags.ifGet("msg-param-displayName", [&](Text value) {
		msgParamDisplayName = value;
	});
	tags.ifGet("msg-param-login", [&](Text value) {
		msgParamLogin = value;
	});
	tags.ifGet("msg-param-months", [&](Text value) {
		msgParamMonths = value.to_uint();
	});
	tags.ifGet("msg-param-recipient-display-name", [&](Text value) {
		msgParamReciptentDisplayName = value;
	});
	tags.ifGet("msg-param-recipient-id", [&](Text value) {
		msgParamRecipientId = value;
	});
	tags.ifGet("msg-param-recipient-user-name", [&](Text value) {
		msgParamRecipientUserName = value;
	});
	tags.ifGet("msg-param-sub-plan", [&](Text value) {
		msgParamSubPlan = value;
	});
	tags.ifGet("msg-param-sub-plan-name", [&](Text value) {
		msgParamSubPlanName = value;
	});
	tags.ifGet("msg-param-viewerCount", [&](Text value) {
		msgParamViewerCount = value;
	});
	tags.ifGet("msg-param-ritual-name", [&](Text value) {
		msgParamRitualName = value;
	});
	tags.ifGet("system-msg", [&](Text value) {
		systemMsg = value;
	});
	tags.ifGet("login", [&](Text value) {
		login = value;
	});
}
void TwitchUserState::parse(ReferenceMap<Text, Text>& tags) noexcept
{
	TwitchUserInfoCommon::parse(tags);
	tags.ifGet("emotes", [&](Text value) {
		for (Text emote : value.splitIterable(','))
		{
			emotes.push(emote.to_uint());
		}
	});
}
void TwitchPrivMsg::parse(ReferenceMap<Text, Text>& tags) noexcept
{
	TwitchUserMessageLike::parse(tags);
	tags.ifGet("bits", [&](Text value) { bits = value.to_uint64(); });
	tags.ifGet("id", [&](Text value) { id = value; });
	tags.ifGet("message", [&](Text value) { message = value; });
	tags.ifGet("room-id", [&](Text value) { roomId = value; });
}
void TwitchUserMessageLike::parse(ReferenceMap<Text, Text>& tags) noexcept
{
	TwitchUserInfoCommon::parse(tags);
	tags.ifGet("tmi-sent-ts", [&](Text value) {
		tmiSentTs = value;
	});
	tags.ifGet("room-id", [&](Text value) {
		roomId = value;
	});
	tags.ifGet("user-id", [&](Text value) {
		userId = value;
	});

	tags.ifGet("emotes", [&](Text value) {
		if (value.empty()) return;
		for (;;)
		{
			//http://static-cdn.jtvnw.net/emoticons/v1/:<emote ID>/:<size>
			//(size is 1.0, 2.0 or 3.0.)

			uint id = value.readwith_e(':').to_uint(10);

			do
			{
				EmoticonInfo * info = emotes.prepare(1);
				info->url.reserve(64);
				info->url << "http://static-cdn.jtvnw.net/emoticons/v1/" << id << "/1.0";
				info->start = value.readwith_e('-').to_uint(10);
				info->end = value.readto_ye(",/").to_uint(10);
				if (value.empty()) goto _end;
			} while (value.read() == ',');
		}
	_end:

		Searcher<EmoticonInfo>::sort(emotes);
	});
	tags.ifGet("badges", [&](Text value) {
		TmpArray<Text> badgeUrls;
		for (Text badgeName : value.splitIterable(','))
		{
			Text type = badgeName.readwith_e('/');
			uint version = badgeName.to_uint();
			Text url = twitch::getBadgeUrl(type, version);
			if (url == nullptr) continue;
			badgeUrls.push(url);
		}
		badges.resize(badgeUrls.size());
		ImageKeeper * image = badges.data();
		for (Text url : badgeUrls)
		{
			image->load(url);
			image++;
		}
	});
}

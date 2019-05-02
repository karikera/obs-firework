#pragma once

#include "imagecache.h"
#include "drawcontext.h"

#include <KR3/data/map.h>
#include <KRMessage/net/irc.h>

using namespace kr;

struct TwitchUserInfo;
struct TwitchUserInfoCommon;

struct TwitchUserInfoCommon
{
	enum class UserType
	{
		none,
		mod,
		global_mod,
		admin,
		staff
	};
	dword color = -1;
	AText displayName;
	bool mod = false;
	bool subscriber = false;
	bool turbo = false;
	UserType userType = UserType::none;

	void parse(ReferenceMap<Text, Text> & tags) noexcept;

	TwitchUserInfo * moveUserInfoTo(irc::User * to) noexcept;
};

struct TwitchUserInfo :TwitchUserInfoCommon
{
	static TwitchUserInfo * get(irc::User * user) noexcept;
};

struct TwitchUserMessageLike :TwitchUserInfoCommon
{
	AText tmiSentTs; // Timestamp when the server received the message.
	AText roomId;
	AText userId;
	Array<EmoticonInfo> emotes; // Information to replace text in the message with emote images. This can be empty. Syntax:
	Array<ImageKeeper> badges;

	void parse(ReferenceMap<Text, Text> & tags) noexcept;
};

struct TwitchPrivMsg :TwitchUserMessageLike
{
	AText id;
	AText message;
	uint64_t bits = 0;

	// bits;
	// (Sent only for Bits messages) The amount of cheer/Bits employed by the user. All instances of these regular expressions:
	// /(^\|\s)<emote-name>\d+(\s\|$)/
	// (where <emote-name> is an emote name returned by the Get Cheermotes endpoint), should be replaced with the appropriate emote:
	// static-cdn.jtvnw.net/bits/<theme>/<type>/<color>/<size>
	//theme – light or dark
	//type – animated or static
	//color – red for 10000+ Bits, blue for 5000-9999, green for 1000-4999, purple for 100-999, gray for 1-99
	//size – A digit between 1 and 4

	void parse(ReferenceMap<Text, Text> & tags) noexcept;
};

struct TwitchUserState :TwitchUserInfoCommon
{
	Array<uint> emotes;

	void parse(ReferenceMap<Text, Text> & tags) noexcept;
};

struct TwitchUserNotice :TwitchUserMessageLike
{
	enum class MsgId
	{
		none,
		sub,
		resub,
		subgift,
		raid,
		ritual
	};

	MsgId msgId = MsgId::none;
	AText login;
	AText msgParamDisplayName; // (Sent only on raid) The display name of the source user raiding this channel.
	AText msgParamLogin; // (Sent on only raid) The name of the source user raiding this channel.
	uint msgParamMonths = 0; // (Sent only on sub or resub) The number of consecutive months the user has subscribed for, in a resub notice.
	AText msgParamReciptentDisplayName; // (Sent only on subgift) The display name of the subscription gift recipient.
	AText msgParamRecipientId; // (Sent only on subgift) The user ID of the subscription gift recipient.
	AText msgParamRecipientUserName; // (Sent only on subgift) The user name of the subscription gift recipient.
	AText msgParamSubPlan; // (Sent only on sub or resub) The type of subscription plan being used. Valid values: Prime, 1000, 2000, 3000. 1000, 2000, and 3000 refer to the first, second, and third levels of paid subscriptions, respectively (currently $4.99, $9.99, and $24.99).
	AText msgParamSubPlanName; // (Sent only on sub or resub) The display name of the subscription plan. This may be a default name or one created by the channel owner.
	AText msgParamViewerCount; // (Sent only on raid) The number of viewers watching the source channel raiding this channel.
	AText msgParamRitualName; // (Sent only on ritual) The name of the ritual this notice is for. Valid value: new_chatter.
	AText systemMsg;

	void parse(ReferenceMap<Text, Text> & tags) noexcept;
};

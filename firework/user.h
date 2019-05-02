#pragma once

#include <KR3/main.h>

using namespace kr;

enum class ServerType
{
	YouTube,
	Twitch,
};

struct ChatUser
{
	ServerType server;
	AText id;

	ChatUser() noexcept;
	ChatUser(ServerType server, AText id) noexcept;
	ChatUser(ChatUser&& _move) noexcept;
	ChatUser(const ChatUser& _copy) noexcept;
	ChatUser(nullptr_t) noexcept;
	ChatUser& operator = (const ChatUser & _copy) noexcept;
	ChatUser& operator = (ChatUser && _move) noexcept;
	ChatUser& operator = (nullptr_t) noexcept;
	bool operator == (nullptr_t) const noexcept;
	bool operator != (nullptr_t) const noexcept;
	bool operator == (const ChatUser & user) const noexcept;
	bool operator != (const ChatUser & user) const noexcept;
};

template <>
struct std::hash<ChatUser>
{
	size_t operator()(const ChatUser& user) const noexcept;
};

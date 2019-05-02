#include "stdafx.h"
#include "user.h"

ChatUser::ChatUser() noexcept
{
}
ChatUser::ChatUser(ServerType server, AText id) noexcept
	:server(server), id(move(id))
{
}
ChatUser::ChatUser(ChatUser&& _move) noexcept
{
	id = move(_move.id);
	server = _move.server;
}
ChatUser::ChatUser(const ChatUser& _copy) noexcept
{
	id = _copy.id;
	server = _copy.server;
}
ChatUser::ChatUser(nullptr_t) noexcept
{
}
ChatUser& ChatUser::operator = (const ChatUser& _copy) noexcept
{
	id = _copy.id;
	server = _copy.server;
	return *this;
}
ChatUser& ChatUser::operator = (ChatUser&& _move) noexcept
{
	id = _move.id;
	server = _move.server;
	return *this;
}
ChatUser& ChatUser::operator = (nullptr_t) noexcept
{
	id = nullptr;
	return *this;
}
bool ChatUser::operator == (nullptr_t) const noexcept
{
	return id == nullptr;
}
bool ChatUser::operator != (nullptr_t) const noexcept
{
	return id != nullptr;
}
bool ChatUser::operator == (const ChatUser& user) const noexcept
{
	return server == user.server && id == user.id;
}
bool ChatUser::operator != (const ChatUser& user) const noexcept
{
	return server != user.server || id != user.id;
}

size_t std::hash<ChatUser>::operator()(const ChatUser& user) const noexcept
{
	return user.id.hash() ^ (size_t)user.server;
}


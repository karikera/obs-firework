#pragma once

#include "drawobject.h"
#include "imagecache.h"
#include "drawcontext.h"
#include "user.h"

#include <KR3/data/map.h>

class ChatCommon;
class ChatList;

class BanListener
{
public:
	virtual void onBan(ChatCommon * chat) noexcept = 0;
};

class ChatCommon: public Referencable<ChatCommon>
{
	friend ChatList;
public:
	ChatCommon(BanListener * onban, ChatUser user, AText imageUrl, AText16 name,
		Text message, d2d::Font font, float boxWidth, 
		size_t loadingCount, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept;
	virtual ~ChatCommon() noexcept;

	CallableList onLoad;

	void startLoad() noexcept;

	bool update(float x, float y, float width, float height) noexcept;
	float drawIcon(float x, float y, float alpha = 1.f) noexcept;
	void ban() noexcept;
	ServerType getServer() noexcept;
	Text getId() noexcept;
	virtual void onLoadEnd() noexcept;

	bool dead;

protected:
	size_t m_loading;
	void _loadOnce() noexcept;

	const ChatUser m_user;
	const AText16 m_name;
	BanListener * m_onban;
	ImageKeeper m_image;
	bool m_floating;
	vec2 m_from;
	vec2 m_to;
	vec2 m_drawPos;
	vec2 m_startSpeed;

	double m_startTime;

	float m_nameHeight;
	float m_badgeWidth;
	float m_lifeTime;
	TextBox m_msgbox;
	Array<ImageKeeper> m_badges;
};

class NormalChat:public ChatCommon, public Node<NormalChat, true>
{
public:
	NormalChat(BanListener * onban, ChatUser user, AText imageUrl, AText16 name, Text message, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept;
	~NormalChat() noexcept override;

	float draw(float y) noexcept;
	void onLoadEnd() noexcept override;
};

class SuperChat:public ChatCommon, public Node<SuperChat, true>
{
public:
	SuperChat(BanListener * onban, ChatUser user, AText imageUrl, AText16 name, Text message, int tier, Text cost, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept;
	~SuperChat() noexcept override;

	uint getWon() noexcept;
	float draw(float y) noexcept;
	void onLoadEnd() noexcept override;

private:
	float _setTierAndReturnWidth(int tier, Text cost) noexcept;

	const AText16 m_cost;
	float m_costWidth;
	float m_textY;

	int m_tier;
	uint m_won;
	bool m_native;
	float m_ttsDuration;
	AText16 m_ttsFilename;
	double m_ttsStartTime;
	bool m_ttsStarted;

	float m_x;
	float m_y;
};

class ChatList
{
public:
	ChatList(BanListener * onban) noexcept;
	~ChatList() noexcept;
	void clear() noexcept;
	void drawAll() noexcept;
	NormalChat* makeNormalChat(ChatUser user, AText imageUrl, Text name, Text message, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept;
	SuperChat* makeDonation(ChatUser user, AText imageUrl, AText16 name, Text message, int tier, Text cost, View<EmoticonInfo> emotes, Array<ImageKeeper> badges) noexcept;

private:
	BanListener * m_onban;
	LinkedList<NormalChat> m_chattings;
	LinkedList<SuperChat> m_donations;
};

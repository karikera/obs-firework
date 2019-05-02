#pragma once

#include <KRApp/dx/d2d.h>
#include <KR3/util/callable.h>
#include <KR3/data/linkedlist.h>

using namespace kr;

class DrawObject: public Node<DrawObject, true>
{
public:
	bool dead;

	DrawObject() noexcept;
	virtual ~DrawObject() noexcept;
	virtual void draw() noexcept;

	static void drawAll() noexcept;
	static void clear() noexcept;
};

class CustomDraw : public DrawObject
{
public:
	CustomDraw(CallablePtr draw) noexcept;
	void draw() noexcept;

private:
	Must<Callable> m_draw;

};

constexpr float GRAVITY = 5.f;

extern LinkedList<DrawObject> g_draws;
extern LinkedList<DrawObject> g_newdraws;

#include "stdafx.h"
#include "drawobject.h"

LinkedList<DrawObject> g_draws;
LinkedList<DrawObject> g_newdraws;

DrawObject::DrawObject() noexcept
	:dead(false)
{
}
DrawObject::~DrawObject() noexcept
{
}
void DrawObject::draw() noexcept
{
}
void DrawObject::drawAll() noexcept
{
	for (DrawObject & obj : g_draws)
	{
		obj.draw();
	}
	g_draws.removeMatchAll([](DrawObject * obj) {
		return obj->dead;
	});
	g_draws.attachMoveAll(&g_newdraws);
}
void DrawObject::clear() noexcept
{
	g_draws.clear();
	g_newdraws.clear();
}

CustomDraw::CustomDraw(CallablePtr draw) noexcept
	:m_draw(draw)
{
}
void CustomDraw::draw() noexcept
{
	try
	{
		m_draw->call();
	}
	catch (ThrowAbort&)
	{
		dead = true;
	}
}

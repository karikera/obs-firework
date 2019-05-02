#include "stdafx.h"
#include "msgobj.h"
#include "drawcontext.h"
#include "drawobject.h"
#include "res.h"

constexpr float FONT_SIZE = 60.f;

StartMessage::StartMessage(const vec4 &color, AText16 text) noexcept
	:m_font(u"¸¼Àº °íµñ", FONT_SIZE), m_color(color), m_text(move(text)), m_timeFrom(g_dc.now)
{
	float width = g_dc->measureText(m_text, m_font);
	
	m_pos.x = (g_dc.width - width) / 2;
	m_pos.y = (g_dc.height - FONT_SIZE) / 2;
}
void StartMessage::draw() noexcept
{
	float t = (float)(g_dc.now - m_timeFrom);
	if (t >= 5.f)
	{
		dead = true;
		return;
	}
	float y = m_pos.y;
	if (t >= 4.f)
	{
		t -= 4.f;
		y -= 20.f * t;
	}
	else
	{
		t = 0.f;
	}
	m_color.a = 1 - t;
	g_dc->fillText(m_text, m_font, { m_pos.x, y }, g_dc->createSolidBrush(m_color));
}

NotifyMessage::NotifyMessage(const vec4 &color, AText16 text) noexcept
	:m_font(u"¸¼Àº °íµñ", FONT_SIZE), m_color(g_dc->createSolidBrush(color)), m_text(move(text)), m_timeFrom(g_dc.now)
{
	float width = g_dc->measureText(m_text, m_font);
	m_pos.x = (g_dc.width - width) / 2;
	m_pos.y = (g_dc.height - FONT_SIZE) / 2;
}
void NotifyMessage::draw() noexcept
{
	g_dc->fillText(m_text, m_font, { m_pos.x, m_pos.y }, m_color);
}


TextMark::TextMark(vec2 pos, const vec4& color, AText16 text) noexcept
	:m_pos(pos), m_color(color), m_text(move(text)), m_timeFrom(g_dc.now)
{
}
void TextMark::draw() noexcept
{
	float t = (float)(g_dc.now - m_timeFrom);
	if (t >= 1.f)
	{
		dead = true;
		return;
	}
	float y = m_pos.y - 20.f * t;
	m_color.a = 1 - t;
	g_dc->fillText(m_text, g_res->miniFont, { m_pos.x, y }, g_dc->createSolidBrush(m_color));
}

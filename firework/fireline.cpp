#include "stdafx.h"
#include "fireline.h"
#include "drawcontext.h"
#include "res.h"
#include "util.h"

FireLine::FireLine(d2d::SolidBrush brush, float lineWidth, vec2 pos) noexcept
	:m_brush(brush), m_lineWidth(lineWidth)
{
	for (vec2& pt : m_pos)
	{
		pt = pos;
	}
}
FireLine::~FireLine() noexcept
{
}
void FireLine::draw() noexcept
{
	// draw lines
	{
		vec2 * prev = m_pos;
		vec2 * iter = m_pos + 1;
		vec2 * end = m_pos + countof(m_pos);
		while (iter != end)
		{
			g_dc->drawLine(*prev, *iter, m_brush, m_lineWidth);
			prev = iter++;
		}
	}

	// shift lines
	{
		vec2 * start = m_pos;
		vec2 * ptr = m_pos + countof(m_pos) - 1;
		for (;;)
		{
			vec2 * prev = ptr - 1;
			*ptr = *prev;
			ptr = prev;
			if (ptr == start) break;
		}
	}

	// gradient
	//float r = 30.f;
	//float r2 = r * 2.f;
	//float x = m_pos[0].X - r;
	//float y = m_pos[0].Y - r;

	//GraphicsPath path;
	//path.AddEllipse(x, y, r2, r2);
	//PathGradientBrush pthGrBrush(&path);
	//Color color;
	//m_pen->GetColor(&color);
	//pthGrBrush.SetCenterColor(color);
	//color.SetValue(color.GetValue() & 0xffffff);
	//int count = 1;
	//pthGrBrush.SetSurroundColors(&color, &count);
	//g_dc->FillEllipse(&pthGrBrush, x, y, r2, r2);
}

Firework::Firework(const vec4 &from, const vec4 &to, float width, vec2 pos, vec2 speed) noexcept
	:FireLine(g_dc->createSolidBrush(clampColor(from)), width, pos), m_speed(speed)
	, m_from(from)
	, m_diff(to - from)
	, m_start(g_dc.now)
{
}
void Firework::draw() noexcept
{
	constexpr float LIFESPAN = 3.f;
	float lifespan = (float)(g_dc.now - m_start) / LIFESPAN;
	if (lifespan >= 1.f)
	{
		dead = true;
		return;
	}

	if (m_pos[countof(m_pos) - 1].y >= g_dc.height) dead = true;

	m_brush.setColor(clampColor(m_diff*lifespan + m_from));

	vec2 next = m_speed * 0.95f;
	next.y += GRAVITY;
	vec2 pos = (m_speed + next) * (0.5f * g_dc.delta) + m_pos[0];

	m_pos[0] = pos;
	m_speed = next;
	FireLine::draw();
}

FireworkMissile::FireworkMissile(d2d::SolidBrush brush, float lineWidth, vec4 expcolor, uint expcount, float exppower, vec2 pos, vec2 speed, double timeTo) noexcept
	:FireLine(brush, lineWidth, pos),
	m_expcolor(expcolor),
	m_expcount(expcount),
	m_exppower(exppower),
	m_speed(speed),
	m_timeTo(timeTo)
{
}
void FireworkMissile::draw() noexcept
{
	if (g_dc.now > m_timeTo)
	{
		vec4 from = m_expcolor + vec4(0.3f, 0.3f, 0.3f, 0.3f);
		vec4 to = m_expcolor * 0.7f;
		to.a = 0.f;

		float angle = math::pi * 2.f / m_expcount;
		for (uint i = 0; i < m_expcount; i++)
		{
			vec2 dir = vec2::direction_z(i * angle)* m_exppower;
			g_newdraws.attach(
				_new Firework(
					from,
					to,
					3.f,
					m_pos[0],
					dir)
			);
		}
		dead = true;
	}
	else
	{
		m_pos[0] += m_speed * g_dc.delta;
		FireLine::draw();
	}

}

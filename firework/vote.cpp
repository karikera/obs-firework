#include "stdafx.h"
#include "vote.h"
#include "res.h"

constexpr float INCREASE_DURATION = 0.5f;

Vote::Pie::Pie(Text pie) noexcept
	:m_name(pie), m_startColor(0, 0, 0, 0), m_latestColor(0, 0, 0, 0)
{
	AddRef();
	m_latestSize = m_startSize = 0.f;
	m_startTime = g_dc.now;
}
Vote::Pie::~Pie() noexcept
{
}
Text Vote::Pie::getName() noexcept
{
	return m_name;
}
void Vote::Pie::draw(const vec4 & colorTo, float fromAngle, float sweep, float r) noexcept
{
	float t = mint((float)(g_dc.now - m_startTime) / INCREASE_DURATION, 1.f);
	float sizeTo = (float)getReferenceCount() - 1.f;
	float size = (sizeTo - m_startSize) * t + m_startSize;
	vec4 color = (colorTo - m_startColor) * t + m_startColor;
	if (m_latestTime != t)
	{
		m_latestTime = t;
		m_latestSize = size;
		m_latestColor = color;
		m_brush.remove();
		m_brush = g_dc->createSolidBrush(color);
	}

	d2d::Path path;
	path.create();
	{
		d2d::PathMake make = path.open();
		make.begin({0.f, 0.f});

		make.lineTo(vec2::direction_z(fromAngle) * r);
		make.arcTo({0, 0}, r, fromAngle, sweep);
		make.end(true);
		make.close();
	}

	g_dc->fill(path, m_brush);
}
void Vote::Pie::increase() noexcept
{
	m_latestTime = 0.f;
	m_startColor = m_latestColor; 
	m_startSize = m_latestSize;
	m_startTime = g_dc.now;
	AddRef();
}
size_t Vote::Pie::decrease() noexcept
{
	m_latestTime = 0.f;
	m_startColor = m_latestColor;
	m_startSize = m_latestSize;
	m_startTime = g_dc.now;
	return Release();
}

Vote::Vote(float radius) noexcept
	:m_radius(radius)
{
}
void Vote::clear() noexcept
{
	for (auto& iter : m_pies)
	{
		delete iter.second;
	}
	m_pies.clear();
	m_userToPie.clear();
}
void Vote::vote(Text pieName, const ChatUser& user) noexcept
{
	auto userInsert = m_userToPie.insert({ user, nullptr });
	if (!userInsert.second)
	{
		Pie* oldpie = userInsert.first->second;
		if (oldpie->getName() == pieName) return;
		if (oldpie->decrease() == 1)
		{
			m_pies.erase(oldpie->getName());
			oldpie->Release();
		}
	}

	Pie* pie;
	auto iter = m_pies.find(pieName);
	if (iter == m_pies.end())
	{
		pie = _new Pie(pieName);
		m_pies.insert(pie->getName(), pie);
	}
	else
	{
		pie = iter->second;
	}

	pie->AddRef();
	userInsert.first->second = pie;
}
void Vote::draw() noexcept
{
	auto _saved = g_dc->save();
	g_dc->setTransform(mat2p::translate(m_radius, m_radius));

	d2d::Path path;
	path.create();
	{
		d2d::PathMake make = path.open();
		make.begin({ 0.f, 0.f });

		float from = (float)math::radianmod(g_dc.now);
		float sweep = (float)fmod(g_dc.now * 0.6, 8.0);
		make.lineTo(vec2::direction_z(from) * m_radius);
		make.arcTo({ 0, 0 }, m_radius, from, sweep);
		make.end(true);
		make.close();
	}

	g_dc->fill(path, g_res->red);
}



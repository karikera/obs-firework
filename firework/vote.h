#pragma once

#include <KR3/data/map.h>
#include <KR3/data/set.h>
#include "user.h"
#include "drawcontext.h"

class Vote
{
public:
	class Pie:public Referencable<Pie>
	{
	public:
		Pie(Text pie) noexcept;
		~Pie() noexcept;

		Text getName() noexcept;
		void draw(const vec4 &colorTo, float fromAngle, float sweep, float r) noexcept;
		void increase() noexcept;
		size_t decrease() noexcept;

	private:
		double m_startTime;

		float m_latestTime;
		float m_startSize;
		float m_latestSize;

		vec4 m_startColor;
		vec4 m_latestColor;

		d2d::SolidBrush m_brush;
		
		AText m_name;
	};
	
	Vote(float radius) noexcept;
	void clear() noexcept;
	void vote(Text pieName, const ChatUser & user) noexcept;
	void draw() noexcept;

private:
	float m_radius;
	Map<ChatUser, Pie*> m_userToPie;
	ReferenceMap<Text, Pie*> m_pies;
	
};

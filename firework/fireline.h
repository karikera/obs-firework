#pragma once

#include "drawobject.h"

class FireLine :public DrawObject
{
protected:
	vec2 m_pos[4];
	d2d::SolidBrush m_brush;
	float m_lineWidth;

	FireLine(d2d::SolidBrush brush, float lineWidth, vec2 pos) noexcept;
	~FireLine() noexcept;
	void draw() noexcept;
};

class Firework :public FireLine
{
private:
	vec2 m_speed;
	vec4 m_from;
	vec4 m_diff;
	double m_start;

public:

	Firework(const vec4 &from, const vec4 &to, float width, vec2 pos, vec2 speed) noexcept;

	void draw() noexcept override;

};

class FireworkMissile :public FireLine
{
private:
	vec4 m_expcolor;
	uint m_expcount;
	float m_exppower;
	vec2 m_speed;
	double m_timeTo;

public:
	FireworkMissile(d2d::SolidBrush brush, float lineWidth, vec4 expcolor, uint expcount, float exppower, vec2 pos, vec2 speed, double timeTo) noexcept;

	void draw() noexcept override;
};

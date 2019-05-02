#pragma once

#include "drawobject.h"

class StartMessage : public DrawObject
{
public:
	StartMessage(const vec4 &color, AText16 text) noexcept;
	void draw() noexcept;

private:
	vec2 m_pos;
	d2d::Font m_font;
	AText16 m_text;
	vec4 m_color;
	double m_timeFrom;

};

class NotifyMessage : public DrawObject
{
public:
	NotifyMessage(const vec4 &color, AText16 text) noexcept;
	void draw() noexcept;

private:
	vec2 m_pos;
	d2d::Font m_font;
	AText16 m_text;
	d2d::Brush m_color;
	double m_timeFrom;

};

class TextMark : public DrawObject
{
public:
	TextMark(vec2 pos, const vec4& color, AText16 text) noexcept;
	void draw() noexcept;

private:
	AText16 m_text;
	vec4 m_color;
	vec2 m_pos;
	double m_timeFrom;

};

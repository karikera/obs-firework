#pragma once

#include <KRApp/dx/d2d.h>

using namespace kr;

constexpr float GRAD_WIDTH = 160;
constexpr float GRAD_END = 60;
constexpr float GRAD_HEIGHT_BASIC = 40.f;
constexpr float GRAD_HEIGHT_EXT = 60.f;
const vec4 GRAD_COLOR(0.f, 0.f, 0.f, 0.5f);

struct ResourceSet
{
	d2d::Layer layer;
	d2d::Font font;
	d2d::Font miniFont;
	d2d::Font normalNameFont;
	d2d::Font normalMessageFont;
	d2d::Font superNameFont;
	d2d::Font superMessageFont;
	d2d::Font sideFont;
	d2d::Bitmap twitchImage;
	d2d::SolidBrush normalChatNameBrush;
	d2d::SolidBrush chatBackBrush;
	d2d::SolidBrush white;
	d2d::SolidBrush gray;
	d2d::SolidBrush black;
	d2d::SolidBrush yellow;
	d2d::SolidBrush red;
	d2d::SolidBrush blue;
	d2d::SolidBrush gradColor;
	d2d::Brush gradBrush;
	d2d::SolidBrush shadowBrush;
	d2d::SolidBrush sideCounter;
	d2d::SolidBrush capturePen;
	d2d::SolidBrush tiers[7];
	d2d::SolidBrush tiersHover[7];

	ResourceSet() noexcept;
};

extern Manual<ResourceSet> g_res;

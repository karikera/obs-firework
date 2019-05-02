#include "stdafx.h"
#include "res.h"
#include "drawcontext.h"
#include "config.h"

Manual<ResourceSet> g_res;

ResourceSet::ResourceSet() noexcept
	:layer(g_dc->createLayer()),
	font(u"³ª´®¼Õ±Û¾¾ Ææ", 60.f),
	miniFont(u"³ª´®¼Õ±Û¾¾ Ææ", 36),
	normalNameFont(u"¸¼Àº °íµñ", g_config.normalChatFontSize, d2d::Font::Weight::Bold),
	normalMessageFont(u"¸¼Àº °íµñ", g_config.normalChatFontSize),
	superNameFont(u"¸¼Àº °íµñ", g_config.superChatFontSize*(18.f/21.f), d2d::Font::Weight::Bold),
	superMessageFont(u"¸¼Àº °íµñ", g_config.superChatFontSize, d2d::Font::Weight::Bold),
	sideFont(u"¸¼Àº °íµñ", 40),
	normalChatNameBrush(g_dc->createSolidBrush({0.25f, 0.25f, 0.7f, 1.f})),
	chatBackBrush(g_dc->createSolidBrush({ 1.f, 1.f, 1.f, 0.5f })),
	white(g_dc->createSolidBrush({ 1.f, 1.f, 1.f, 1.f })),
	gray(g_dc->createSolidBrush({ 0.5f, 0.5f, 0.5f, 1.f })),
	black(g_dc->createSolidBrush({ 0.f, 0.f, 0.f, 1.f })),
	yellow(g_dc->createSolidBrush({ 1.f, 1.f, 0.f, 1.f })),
	red(g_dc->createSolidBrush({ 1.f, 0.f, 0.f, 1.f })),
	blue(g_dc->createSolidBrush({ 0.f, 0.f, 1.f, 1.f })),
	gradColor(g_dc->createSolidBrush(GRAD_COLOR)),
	gradBrush(g_dc->createLinearGradient(
		{ GRAD_WIDTH, 0.f },
		{ GRAD_WIDTH + GRAD_END, 0 },
		{
			{0, GRAD_COLOR }, {1, vec4{ 0,0,0,0 }
		} 
	})),
	shadowBrush(g_dc->createSolidBrush({ 0, 0, 0, 0.5f })),
	sideCounter(g_dc->createSolidBrush({ 1.f, 0.25f, 0.25f, 1.f})),
	capturePen(g_dc->createSolidBrush({ 1.f, 0.5f, 0, 1.f })),
	tiers{ 
		g_dc->createSolidBrush(vec4{19,74,158,128} / 255.f),
		g_dc->createSolidBrush(vec4{40,229,253,128} / 255.f),
		g_dc->createSolidBrush(vec4{50,232,183,128} / 255.f),
		g_dc->createSolidBrush(vec4{252,215,72,128} / 255.f),
		g_dc->createSolidBrush(vec4{243,124,34,128} / 255.f),
		g_dc->createSolidBrush(vec4{230,37,101,128} / 255.f),
		g_dc->createSolidBrush(vec4{227,37,36,128} / 255.f)
	},
	tiersHover{
		g_dc->createSolidBrush(vec4{19,74,158,255} / 255.f),
		g_dc->createSolidBrush(vec4{40,229,253,255} / 255.f),
		g_dc->createSolidBrush(vec4{50,232,183,255} / 255.f),
		g_dc->createSolidBrush(vec4{252,215,72,255} / 255.f),
		g_dc->createSolidBrush(vec4{243,124,34,255} / 255.f),
		g_dc->createSolidBrush(vec4{230,37,101,255} / 255.f),
		g_dc->createSolidBrush(vec4{227,37,36,255} / 255.f)
	}
{
	if (g_config.twitchImage != nullptr)
	{
		twitchImage = g_dc->loadImage(g_config.twitchImage.c_str());
	}
}

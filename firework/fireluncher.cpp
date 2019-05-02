#include "stdafx.h"
#include "fireluncher.h"
#include "drawcontext.h"
#include "drawobject.h"
#include "firework.h"
#include "fireline.h"
#include "msgobj.h"
#include "util.h"
#include "res.h"

constexpr int SAME_COST_COUNT = 6;
const uint COST_INCREMENT[] = { 10, 12, 14, 16, 18, 20, 24, 28, 32, 36, 40, 45, 50, 60, 70, 80 };

double g_nextSideMissile;
uint g_sideMissileCap;
uint g_donateCap;
bool g_launching;
uint g_missileCost = 500;
uint g_missileCostBase = 500;
uint g_incrementIndex;
uint g_sameCostCounter;

double g_sideMissileTimer;
double g_majorMissileTimer;

double g_gradOpenTime;
int g_gradOpenState;

void luncher::init() noexcept
{
	g_majorMissileTimer = g_sideMissileTimer = g_nextSideMissile = g_dc.now;
}
void luncher::missile(float x, vec2 to, const vec4 &color, uint count, float power) noexcept
{
	float speed = g_random.get<float>() * 300 + 600;

	vec2 from = { x, (float)g_dc.height };
	to -= from;
	float mul = speed / to.length();
	to *= mul;

	vec4 mcolor = color;
	mcolor.a *= 0.2f;

	g_newdraws.attach(_new FireworkMissile(
		g_dc->createSolidBrush(mcolor), 2.f,
		color,
		count,
		power,
		from, to,
		g_dc.now + 1.0 / mul));
}
void luncher::majorMissile() noexcept
{
	float v = g_random.get<float>()*6.f;
	vec4 color;
	switch ((int)v)
	{
	case 0: color = { 1, v, 0, 1 }; break;
	case 1: color = { 2 - v, 1, 0, 1 }; break;
	case 2: color = { 0, 1, v - 2, 1 }; break;
	case 3: color = { 0, 4 - v, 1, 1 }; break;
	case 4: color = { v - 4, 0, 1, 1 }; break;
	case 5: color = { 1, 0, 6 - v, 1 }; break;
	}
	float randomX = 400;

	float x = (g_dc.width - randomX)*0.5f + g_random.get<float>() * randomX;

	vec2 targetRandom = { 200.f, 100.f };
	vec2 size = { (float)g_dc.width, (float)g_dc.height };
	vec2 randval = { g_random.get<float>() , g_random.get<float>() };
	vec2 to = (size - targetRandom) * 0.5f + targetRandom * randval;
	to.y += 100.f;
	uint count = g_random.get<uint>() % 5 + 40;
	missile(x, to, color, count, g_random.get<float>() * 50.f + 600.f);
}
void luncher::sideMissile() noexcept
{
	float v = g_random.get<float>()*6.f;
	vec4 color;
	switch ((int)v)
	{
	case 0: color = { 1, v, 0, 1 }; break;
	case 1: color = { 2 - v, 1, 0, 1 }; break;
	case 2: color = { 0, 1, v - 2, 1 }; break;
	case 3: color = { 0, 4 - v, 1, 1 }; break;
	case 4: color = { v - 4, 0, 1, 1 }; break;
	case 5: color = { 1, 0, 6 - v, 1 }; break;
	}
	float randomX = 1300;

	float x = (g_dc.width - randomX)*0.5f + g_random.get<float>() * randomX;

	vec2 targetRandom = { 100.f, 100.f };
	vec2 randval = { g_random.get<float>() , g_random.get<float>() };
	vec2 to = targetRandom * -0.5f + targetRandom * randval;
	to.x += x;
	to.y += (float)g_dc.height - 200.f;

	uint count = g_random.get<uint>() % 5 + 10;
	missile(x, to, color, count, g_random.get<float>()*100.f + 150.f);
}
bool luncher::buyMissile() noexcept
{
	uint cost = g_missileCost;
	if (g_donateCap < cost) return false;
	g_donateCap -= cost;
	g_newdraws.attach(_new TextMark({ 120.f, g_dc.height - 100.f }, {1,1,1,1}, AText16() << u'-' << cost << u'£Ü'));

	g_sameCostCounter++;
	if (g_sameCostCounter >= SAME_COST_COUNT)
	{
		g_sameCostCounter = 0;
		g_incrementIndex++;
		if (g_incrementIndex >= countof(COST_INCREMENT))
		{
			g_incrementIndex = 0;
			g_missileCostBase *= 10;
		}
		g_missileCost = g_missileCostBase * COST_INCREMENT[g_incrementIndex] / 10;
	}

	return true;
}
void luncher::chargeValue(uint won) noexcept
{
	g_newdraws.attach(_new TextMark({ 120.f, g_dc.height - 100.f }, {1,1,1,1}, AText16() << u'+' << won << u'£Ü'));
	g_donateCap += won;
	if (!g_launching)
	{
		g_majorMissileTimer = g_dc.now + 1.0;
		g_launching = true;
	}
	openGradView();
}
void luncher::openGradView() noexcept
{
	if (g_gradOpenState != 0) return;
	g_gradOpenState = 1;
	g_gradOpenTime = g_dc.now;
}
void luncher::drawGradView() noexcept
{
	// lunch
	if (g_dc.now > g_majorMissileTimer)
	{
		if (g_gradOpenState == 2)
		{
			if (buyMissile())
			{
				g_majorMissileTimer = g_dc.now + 0.5;
				majorMissile();
				g_launching = true;
			}
		}
	}
	if (g_launching)
	{
		if (g_dc.now > g_sideMissileTimer)
		{
			if (g_sideMissileCap != 0)
			{
				// g_newdraws.attach(_new TextMark({ 40.f, g_dc.height - 40.f }, {1,0,0,1}, u"-1"));
				g_sideMissileTimer = g_dc.now + 0.1;
				g_sideMissileCap--;
				sideMissile();
			}
			else
			{
				if (g_donateCap < g_missileCost)
				{
					g_launching = false;
					double next = g_dc.now + 1.f;
					if (next < g_nextSideMissile) g_nextSideMissile = next;
				}
			}
		}
	}
	if (g_dc.now > g_nextSideMissile)
	{
		// g_newdraws.attach(_new TextMark({ 40.f, g_dc.height - 40.f }, {1,0,0,1}, u"+1"));
		g_nextSideMissile += 1.0 + g_sideMissileCap / 4.0;
		g_sideMissileCap++;
	}

	// draw
	return;
	float gradHeight;
	float openTime = 0.f;
	switch (g_gradOpenState)
	{
	case 0:
		gradHeight = GRAD_HEIGHT_BASIC;
		break;
	case 1:
		openTime = (float)(g_dc.now - g_gradOpenTime) * 2;
		gradHeight = openTime * GRAD_HEIGHT_EXT + GRAD_HEIGHT_BASIC;
		if (gradHeight > GRAD_HEIGHT_BASIC + GRAD_HEIGHT_EXT)
		{
			openTime = 1.f;
			gradHeight = GRAD_HEIGHT_BASIC + GRAD_HEIGHT_EXT;
			g_gradOpenState = 2;
		}
		break;
	default:
		gradHeight = GRAD_HEIGHT_BASIC + GRAD_HEIGHT_EXT;
		break;
	}
	float gradY = (float)g_dc.height - gradHeight;
	g_dc->fillRect({ 0.f, gradY, GRAD_WIDTH, gradY + gradHeight }, g_res->gradColor);
	g_dc->fillRect({ GRAD_WIDTH, gradY, GRAD_WIDTH + GRAD_END, gradY + gradHeight }, g_res->gradBrush);

	TText16 text;
	text << g_sideMissileCap;
	g_dc->fillText(text, g_res->font, { 0, g_dc.height - 50.f }, g_res->sideCounter);
	text.clear();

	d2d::SolidBrush brush;
	if (g_gradOpenState == 1)
	{
		brush = g_dc->createSolidBrush({ 1,1,1,openTime });
	}
	else if (g_gradOpenState == 2)
	{
		brush = g_res->white;
	}

	if (brush != nullptr)
	{
		text.clear();
		text << u"°¡°Ý " << g_missileCost << u"£Ü ";
		text << fill(u'/', SAME_COST_COUNT - g_sameCostCounter);
		g_dc->fillText(text, g_res->miniFont, { 0, g_dc.height - GRAD_HEIGHT_BASIC - 30.f }, brush);

		text.clear();
		text << u"ÀÜ°í " << g_donateCap << u'£Ü';
		g_dc->fillText(text, g_res->miniFont, { 0.f, g_dc.height - GRAD_HEIGHT_BASIC - 60.f }, brush);
	}
}

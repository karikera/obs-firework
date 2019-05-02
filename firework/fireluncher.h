#pragma once

using namespace kr;

namespace luncher
{
	void init() noexcept;
	void missile(float x, vec2 to, const vec4 &color, uint count, float power) noexcept;
	void majorMissile() noexcept;
	void sideMissile() noexcept;
	bool buyMissile() noexcept;
	void chargeValue(uint won) noexcept;
	void openGradView() noexcept;
	void drawGradView() noexcept;
}

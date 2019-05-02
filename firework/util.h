#pragma once

#include <KR3/math/coord.h>

using namespace kr;

const vec4 clamp(const vec4 & a, const vec4 & v, const vec4 & b) noexcept;

const vec4 clampColor(const vec4& v) noexcept;

AText16 getTempFileName(Text16 ext) noexcept;

#include "stdafx.h"
#include "util.h"
#include <KRUtil/fs/file.h>

const vec4 clamp(const vec4 & a, const vec4 & v, const vec4 & b) noexcept
{
	return vec4(
		kr::math::clamp(a.x, v.x, b.x),
		kr::math::clamp(a.y, v.y, b.y),
		kr::math::clamp(a.z, v.z, b.z),
		kr::math::clamp(a.w, v.w, b.w)
	);
}

const vec4 clampColor(const vec4& v) noexcept
{
	return vec4(
		kr::math::clamp(0.f, v.x, 1.f),
		kr::math::clamp(0.f, v.y, 1.f),
		kr::math::clamp(0.f, v.z, 1.f),
		kr::math::clamp(0.f, v.w, 1.f)
	);
}

AText16 getTempFileName(Text16 ext) noexcept
{
	static uint fileidx = 0;
	AText16 filename;
	filename << u"temp/";

	size_t basicNameIdx = filename.size();
	for (;;)
	{
		filename << fileidx << u'.' << ext;
		fileidx++;
		filename.c_str();
		if (!File::exists(filename.data())) break;
		filename.cut_self(basicNameIdx);
	}
	return filename;
}

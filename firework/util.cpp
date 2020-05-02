#include "stdafx.h"
#include "util.h"
#include <KRUtil/fs/file.h>

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

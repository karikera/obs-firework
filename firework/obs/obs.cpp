#include "stdafx.h"
#ifndef FIREWORK_EXE_BUILD

#include "obs.h"
#include "source.h"
#include "imagesource.h"
#include "obs_firework.h"

#include <KR3/initializer.h>
#include <KRUtil/net/socket.h>
#include <KRApp/dx/wic.h>
#include <KRMessage/pool.h>
#include <KRSound/mf.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("firework", "en-US")

#pragma comment(lib, "obs.lib")
#pragma comment(lib, "libobs-d3d11.lib")

#define blog(log_level, format, ...) \
	blog(log_level, "[firework: '%s'] " format, \
			obs_source_get_name(m_source), ##__VA_ARGS__)

namespace
{
	kr::Manual<kr::Initializer<kr::COM, kr::Socket, kr::WICBitmapDecoder, kr::MFSourceReader>> s_init;
}


extern "C" ATTR_EXPORT bool obs_module_load(void)
{
	s_init.create();
	OBSSourceInfo<OBSFirework>::init();
	OBSSourceInfo<ImageSourceExample>::init();
	return true;
}

extern "C" ATTR_EXPORT void obs_module_unload(void)
{
	OBSSourceInfo<OBSFirework>::deinit();
	OBSSourceInfo<ImageSourceExample>::deinit();

	s_init.remove();
	kr::StackAllocator::getInstance()->terminate();
	kr::ThreadPool::getInstance()->terminate();
}

#endif

#include "stdafx.h"
#ifndef FIREWORK_EXE_BUILD
#include "device.h"
#include "obs.h"

#include <KR3/initializer.h>
#include <KRApp/dx/d3d11.h>
#include <KRApp/dx/d2d.h>

namespace
{
	kr::Manual<kr::Initializer<kr::d2d::Direct2D>> s_init2;
}


void resetDevice() noexcept
{
	obs_enter_graphics();
	ID3D11Device * device = gs_get_context()->device->device;
	obs_leave_graphics();

	auto * oldDevice = kr::d3d11::Direct3D11::getDevice();
	if (device == oldDevice) return;
	if (oldDevice) s_init2.remove();
	kr::d3d11::Direct3D11::setDevice(device);
	s_init2.create();
}
void clearDevice() noexcept
{
	s_init2.remove();
	kr::d3d11::Direct3D11::setDevice(nullptr);
}
#endif

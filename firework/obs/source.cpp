#include "stdafx.h"

#ifndef FIREWORK_EXE_BUILD
#include "source.h"

void * OBSSource::operator new(size_t size) noexcept
{
	return bmalloc(size);
}

void OBSSource::operator delete(void * data) noexcept
{
	bfree(data);
}

OBSSource::OBSSource(obs_data_t *settings, obs_source_t *source) noexcept
	:m_source(source)
{
}

OBSSource::~OBSSource() noexcept
{
}

void OBSSource::update(obs_data_t *settings) noexcept
{
}

void OBSSource::show() noexcept
{
}

void OBSSource::hide() noexcept
{
}

uint32_t OBSSource::getWidth() noexcept
{
	return 0;
}

uint32_t OBSSource::getHeight() noexcept
{
	return 0;
}

void OBSSource::videoRender(gs_effect_t *effect) noexcept
{
}

bool OBSSource::audioRender(uint64_t* ts_out, obs_source_audio_mix* audio_output, uint32_t mixers, size_t channels, size_t sample_rate) noexcept
{
	return false;
}

void OBSSource::tick(float seconds) noexcept
{
}

void OBSSource::mouseClick(const obs_mouse_event* event,
	int32_t type, bool mouse_up, uint32_t click_count) noexcept
{	
}

void OBSSource::mouseMove(const obs_mouse_event* event, bool mouse_leave) noexcept
{
}

obs_properties_t *OBSSource::getProperties() noexcept
{
	obs_properties_t *props = obs_properties_create();
	return props;
}

const char *OBSSource::getName() noexcept
{
	return obs_module_text("Unnamed");
}

void OBSSource::defaults(obs_data_t *settings) noexcept
{
}

void OBSSource::init() noexcept
{
}

void OBSSource::deinit() noexcept
{
}

#endif
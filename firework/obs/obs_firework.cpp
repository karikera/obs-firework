#include "stdafx.h"
#ifndef FIREWORK_EXE_BUILD

#include "obs_firework.h"
#include "firework.h"
#include "device.h"
#include "mfmix.h"

const char * OBSFirework::getId() noexcept
{
	return "firework";
}

void OBSFirework::load() noexcept
{
	if (m_texture != nullptr) return;

	obs_enter_graphics();
	m_texture = gs_texture_create(m_width, m_height, GS_RGBA, 1, nullptr, GS_RENDER_TARGET);
	obs_leave_graphics();
}

void OBSFirework::unload() noexcept
{
	if (m_texture == nullptr) return;

	obs_enter_graphics();
	gs_texture_destroy(m_texture);
	m_texture = nullptr;
	obs_leave_graphics();
}

OBSFirework::OBSFirework(obs_data_t * settings, obs_source_t * source) noexcept
	:OBSSource(settings, source)
{
	m_texture = nullptr;
	m_firework = FireworkSystem::create((double)obs_get_video_frame_time() / 1000000000);
	g_mix.setCallback(
		[] { return (double)obs_get_video_frame_time() / 1000000000; },
		[this](const WaveFormat * format, Buffer data, double time) {
		obs_source_audio audio;
		audio.speakers = (speaker_layout)format->channels;
		audio.samples_per_sec = format->samplesPerSec;
		switch (format->bitsPerSample)
		{
		case 8: audio.format = AUDIO_FORMAT_U8BIT; break;
		case 16: audio.format = AUDIO_FORMAT_16BIT; break;
		case 32: audio.format = AUDIO_FORMAT_32BIT; break;
		default:
			_assert(!"Invalid bit per sample");
			break;
		}

		audio.data[0] = (uint8_t*)data.data();
		audio.data[1] = nullptr;
		audio.data[2] = nullptr;
		audio.data[3] = nullptr;
		audio.data[4] = nullptr;
		audio.data[5] = nullptr;
		audio.data[6] = nullptr;
		audio.data[7] = nullptr;
		audio.frames = intact<uint32_t>(data.size()) / format->blockAlign;
		audio.timestamp = (uint64_t)(time * 1000000000);

		obs_source_output_audio(m_source, &audio);
	});
	m_unloaded = false;
	m_active = false;
	m_width = 0;
	m_height = 0;

	update(settings);
}

OBSFirework::~OBSFirework() noexcept
{
	unload();

	delete m_firework;
	m_firework = nullptr;
	clearDevice();
}

void OBSFirework::update(obs_data_t * settings) noexcept
{
	int width = (int)obs_data_get_int(settings, "width");
	int height = (int)obs_data_get_int(settings, "height");
	m_unloaded = obs_data_get_bool(settings, "unload");

	if (m_unloaded)
	{
		m_width = width;
		m_height = height;
		unload();
		
		return;
	}
	if (m_width != width || m_height != height)
	{
		m_width = width;
		m_height = height;
		unload();
	}
	load();
}

void OBSFirework::show() noexcept
{
}

void OBSFirework::hide() noexcept
{
}

uint32_t OBSFirework::getWidth() noexcept
{
	return m_width;
}

uint32_t OBSFirework::getHeight() noexcept
{
	return m_height;
}

void OBSFirework::videoRender(gs_effect_t * effect) noexcept
{
	m_firework->render(obs_get_video_frame_time() / 1000000000.0);
	if (m_texture)
	{
		gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), m_texture);
		gs_device_t * device = gs_get_context()->device;
		auto old = device->blendState.srcFactorC;
		device->blendState.srcFactorC = GS_BLEND_ONE;
		device->blendStateChanged = true;
		//device->UpdateBlendState();
		gs_draw_sprite(m_texture, 0, m_width, m_height);
		device->blendState.srcFactorC = old;
		device->blendStateChanged = true;
		//device->UpdateBlendState();
	}
}

void OBSFirework::tick(float seconds) noexcept
{
	m_active = obs_source_active(m_source);
	if (m_texture == nullptr)
	{
		if (!m_destroyed)
		{
			m_firework->destroy();
			m_destroyed = true;
		}
	}
	else
	{
		resetDevice();
		m_firework->createWindowFrom(static_cast<gs_texture_2d*>(m_texture)->texture, m_width, m_height);
		m_destroyed = false;
	}
	m_firework->update();
}

void OBSFirework::mouseClick(const obs_mouse_event* event,
	int32_t type, bool mouse_up, uint32_t click_count) noexcept
{
	if (mouse_up)
	{
		m_firework->mouseUp();
	}
	else
	{
		m_firework->mouseDown();
	}
}

void OBSFirework::mouseMove(const obs_mouse_event* event, bool mouse_leave) noexcept
{
	if (mouse_leave)
	{
		m_firework->mouseOut();
		if (event->x == 0 && event->y == 0) return;
	}
	m_firework->mouseMove({ event->x, event->y });
}

obs_properties_t * OBSFirework::getProperties() noexcept
{
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_int(props,
		"width", obs_module_text("Width"),
		0, 0x7fffffff, 1);
	obs_properties_add_int(props,
		"height", obs_module_text("Height"),
		0, 0x7fffffff, 1);
	obs_properties_add_bool(props,
		"unload", obs_module_text("Unload"));
	return props;
}

const char * OBSFirework::getName() noexcept
{
	return obs_module_text("rua.kr Firework");
}

void OBSFirework::defaults(obs_data_t * settings) noexcept
{
	obs_data_set_default_bool(settings, "unload", false);
	obs_data_set_default_int(settings, "width", 1280);
	obs_data_set_default_int(settings, "height", 720);
}

#endif
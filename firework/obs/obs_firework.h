#pragma once

#include "obs.h"
#include "source.h"

#include <KR3/wl/eventhandle.h>
#include <KR3/wl/threadhandle.h>

class FireworkSystem;

class OBSFirework :public OBSSource
{
public:
	static constexpr bool hasVideo = true;
	static constexpr bool hasAudio = true;
	static constexpr bool doNotDupplicate = true;
	static constexpr bool interaction = true;

	static const char * getId() noexcept;

	void load() noexcept;

	void unload() noexcept;

	OBSFirework(obs_data_t *settings, obs_source_t *source) noexcept;

	~OBSFirework() noexcept;

	void update(obs_data_t *settings) noexcept;

	void show() noexcept;

	void hide() noexcept;

	uint32_t getWidth() noexcept;

	uint32_t getHeight() noexcept;

	void videoRender(gs_effect_t *effect) noexcept;

	void tick(float seconds) noexcept;

	void mouseClick(const obs_mouse_event* event,
		int32_t type, bool mouse_up, uint32_t click_count) noexcept;

	void mouseMove(const obs_mouse_event* event, bool mouse_leave) noexcept;

	obs_properties_t *getProperties() noexcept;

	static const char *getName() noexcept;

	static void defaults(obs_data_t *settings) noexcept;

private:
	gs_texture* m_texture;
	FireworkSystem* m_firework;

	int m_width;
	int m_height;

	bool m_unloaded;
	bool m_active;
	bool m_destroyed;

};

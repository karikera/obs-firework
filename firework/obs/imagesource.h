#pragma once

#include "obs.h"
#include "source.h"

class ImageSourceExample :public OBSSource
{
public:
	static constexpr bool hasVideo = true;

	static const char * getId() noexcept;

	void load() noexcept;

	void unload() noexcept;

	ImageSourceExample(obs_data_t *settings, obs_source_t *source) noexcept;

	~ImageSourceExample() noexcept;

	void update(obs_data_t *settings) noexcept;

	void show() noexcept;

	void hide() noexcept;

	uint32_t getWidth() noexcept;

	uint32_t getHeight() noexcept;

	void renderVideo(gs_effect_t *effect) noexcept;

	void tick(float seconds) noexcept;

	obs_properties_t *getProperties() noexcept;

	static const char *getName() noexcept;

	static void defaults(obs_data_t *settings) noexcept;

private:
	char* m_file;
	bool         m_persistent;
	time_t       m_file_timestamp;
	float        m_update_time_elapsed;
	uint64_t     m_last_time;
	bool         m_active;

	gs_image_file_t m_image;

};


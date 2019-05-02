#include "stdafx.h"
#ifndef FIREWORK_EXE_BUILD
#include "imagesource.h"

static time_t get_modified_timestamp(const char *filename) noexcept
{
	struct stat stats;
	if (os_stat(filename, &stats) != 0)
		return -1;
	return stats.st_mtime;
}

static const char *image_filter =
"All formats (*.bmp *.tga *.png *.jpeg *.jpg *.gif);;"
"BMP Files (*.bmp);;"
"Targa Files (*.tga);;"
"PNG Files (*.png);;"
"JPEG Files (*.jpeg *.jpg);;"
"GIF Files (*.gif)";

const char * ImageSourceExample::getId() noexcept
{
	return "obs_example";
}
void ImageSourceExample::load() noexcept
{
	char *file = m_file;

	obs_enter_graphics();
	gs_image_file_free(&m_image);
	obs_leave_graphics();

	if (file && *file) {
		blog(LOG_DEBUG, "loading texture '%s'", file);
		m_file_timestamp = get_modified_timestamp(file);
		gs_image_file_init(&m_image, file);
		m_update_time_elapsed = 0;

		obs_enter_graphics();
		gs_image_file_init_texture(&m_image);
		obs_leave_graphics();

		//m_image.texture;
		//gs_texture;

		if (!m_image.loaded)
			blog(LOG_WARNING, "failed to load texture '%s'", file);
	}
}

void ImageSourceExample::unload() noexcept
{
	obs_enter_graphics();
	gs_image_file_free(&m_image);
	obs_leave_graphics();
}

ImageSourceExample::ImageSourceExample(obs_data_t *settings, obs_source_t *source) noexcept
	:OBSSource(settings, source)
{
	m_file = nullptr;
	m_persistent = false;
	m_file_timestamp = 0;
	m_update_time_elapsed = 0;
	m_last_time = 0;
	m_active = false;
	memset(&m_image, 0, sizeof(gs_image_file_t));

	update(settings);
}

ImageSourceExample::~ImageSourceExample() noexcept
{
	unload();
}

void ImageSourceExample::update(obs_data_t *settings) noexcept
{
	const char *file = obs_data_get_string(settings, "file");
	const bool isUnloaded = obs_data_get_bool(settings, "unload");

	if (m_file)
		bfree(m_file);
	m_file = bstrdup(file);
	m_persistent = !isUnloaded;

	/* Load the image if the source is persistent or showing */
	if (m_persistent || obs_source_showing(m_source))
		load();
	else
		unload();
}

void ImageSourceExample::show() noexcept
{
	if (!m_persistent)
		load();
}

void ImageSourceExample::hide() noexcept
{
	if (!m_persistent)
		unload();
}

uint32_t ImageSourceExample::getWidth() noexcept
{
	return m_image.cx;
}

uint32_t ImageSourceExample::getHeight() noexcept
{
	return m_image.cy;
}

void ImageSourceExample::renderVideo(gs_effect_t *effect) noexcept
{
	if (!m_image.texture) return;

	gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), m_image.texture);
	gs_draw_sprite(m_image.texture, 0, m_image.cx, m_image.cy);
}

void ImageSourceExample::tick(float seconds) noexcept
{
	uint64_t frame_time = obs_get_video_frame_time();

	m_update_time_elapsed += seconds;

	if (m_update_time_elapsed >= 1.0f) {
		time_t t = get_modified_timestamp(m_file);
		m_update_time_elapsed = 0.0f;

		if (m_file_timestamp != t) {
			load();
		}
	}

	if (obs_source_active(m_source)) {
		if (!m_active) {
			if (m_image.is_animated_gif)
				m_last_time = frame_time;
			m_active = true;
		}

	}
	else {
		if (m_active) {
			if (m_image.is_animated_gif) {
				m_image.cur_frame = 0;
				m_image.cur_loop = 0;
				m_image.cur_time = 0;

				obs_enter_graphics();
				gs_image_file_update_texture(&m_image);
				obs_leave_graphics();
			}

			m_active = false;
		}

		return;
	}

	if (m_last_time && m_image.is_animated_gif) {
		uint64_t elapsed = frame_time - m_last_time;
		bool updated = gs_image_file_tick(&m_image, elapsed);

		if (updated) {
			obs_enter_graphics();
			gs_image_file_update_texture(&m_image);
			obs_leave_graphics();
		}
	}

	m_last_time = frame_time;
}

obs_properties_t *ImageSourceExample::getProperties() noexcept
{
	struct dstr path = { 0 };

	obs_properties_t *props = obs_properties_create();

	if (this != nullptr && m_file && *m_file) {
		const char *slash;

		dstr_copy(&path, m_file);
		dstr_replace(&path, "\\", "/");
		slash = strrchr(path.array, '/');
		if (slash)
			dstr_resize(&path, slash - path.array + 1);
	}

	obs_properties_add_path(props,
		"file", obs_module_text("File"),
		OBS_PATH_FILE, image_filter, path.array);
	obs_properties_add_bool(props,
		"unload", obs_module_text("UnloadWhenNotShowing"));
	dstr_free(&path);

	return props;
}

const char *ImageSourceExample::getName() noexcept
{
	return obs_module_text("ImageInput");
}

void ImageSourceExample::defaults(obs_data_t *settings) noexcept
{
	obs_data_set_default_bool(settings, "unload", false);
}
#endif

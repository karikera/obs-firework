#pragma once

#include "obs.h"



class OBSSource
{
public:
	static constexpr bool hasVideo = false;
	static constexpr bool hasAudio = false;
	static constexpr bool doNotDupplicate = false;
	static constexpr bool compositeAudio = false;
	static constexpr bool interaction = false;

	static void * operator new(size_t size) noexcept;

	static void operator delete(void * data) noexcept;

	OBSSource(obs_data_t *settings, obs_source_t *source) noexcept;

	~OBSSource() noexcept;

	void update(obs_data_t *settings) noexcept;

	void show() noexcept;

	void hide() noexcept;

	uint32_t getWidth() noexcept;

	uint32_t getHeight() noexcept;

	void videoRender(gs_effect_t *effect) noexcept;

	bool audioRender(uint64_t* ts_out, obs_source_audio_mix* audio_output, uint32_t mixers, size_t channels, size_t sample_rate) noexcept;
	
	void tick(float seconds) noexcept;

	void mouseClick(const obs_mouse_event* event,
		int32_t type, bool mouse_up, uint32_t click_count) noexcept;

	void mouseMove(const obs_mouse_event* event, bool mouse_leave) noexcept;

	obs_properties_t *getProperties() noexcept;

	static const char *getName() noexcept;

	static void defaults(obs_data_t *settings) noexcept;

	static void init() noexcept;

	static void deinit() noexcept;

protected:
	obs_source_t * m_source;
};

template <typename T>
class OBSSourceInfo :public obs_source_info
{
public:
	OBSSourceInfo() noexcept
	{
		id = T::getId();
		type = OBS_SOURCE_TYPE_INPUT;
		output_flags = 0;
		if (T::hasVideo) { output_flags |= OBS_SOURCE_VIDEO; }
		if (T::hasAudio) { output_flags |= OBS_SOURCE_AUDIO; }
		if (T::doNotDupplicate) { output_flags |= OBS_SOURCE_DO_NOT_DUPLICATE; }
		if (T::compositeAudio) { output_flags |= OBS_SOURCE_COMPOSITE; }
		if (T::interaction) { output_flags |= OBS_SOURCE_INTERACTION; }

		get_name = [](void *unused)->const char * {
			UNUSED_PARAMETER(unused);
			return T::getName();
		};
		create = [](obs_data_t *settings, obs_source_t *source)->void* { return new T(settings, source); };
		destroy = [](void * data)->void { delete (T*)data; };
		update = [](void * data, obs_data_t *settings)->void { return ((T*)data)->update(settings); };
		get_defaults = [](obs_data_t *settings)->void { return T::defaults(settings); };
		if (T::hasVideo)
		{
			show = [](void* data)->void { return ((T*)data)->show(); };
			hide = [](void* data)->void { return ((T*)data)->hide(); };
			get_width = [](void* data)->uint32_t { return ((T*)data)->getWidth(); };
			get_height = [](void* data)->uint32_t { return ((T*)data)->getHeight(); };
			video_render = [](void* data, gs_effect_t * effect)->void { return ((T*)data)->videoRender(effect); };
			video_tick = [](void* data, float seconds)->void { return ((T*)data)->tick(seconds); };
		}
		if (T::interaction)
		{
			mouse_click = [](void* data,
				const obs_mouse_event * event,
				int32_t type, bool mouse_up, uint32_t click_count)->void {
					return ((T*)data)->mouseClick(event, type, mouse_up, click_count);
			};

			mouse_move = [](void* data,
				const obs_mouse_event * event,
				bool mouse_leave)->void {
					return ((T*)data)->mouseMove(event, mouse_leave);
			};
		}
		if (T::compositeAudio)
		{ 
			audio_render = [](void* data, uint64_t * ts_out, obs_source_audio_mix* audio_output,
				uint32_t mixers, size_t channels, size_t sample_rate) -> bool { return ((T*)data)->audioRender(ts_out, audio_output, mixers, channels, sample_rate); };
		}
		get_properties = [](void *data)->obs_properties_t* { return ((T*)data)->getProperties(); };
	}

	static void init() noexcept
	{
		static OBSSourceInfo<T> info;
		obs_register_source(&info);
		T::init();
	}

	static void deinit() noexcept
	{
		T::deinit();
	}
};

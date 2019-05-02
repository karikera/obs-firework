#pragma once

#include "drawobject.h"
#include "mfmix.h"
#include <KRUtil/resloader.h>
#include <KRSound/fft.h>

class MusicPlayer
{
public:
	MusicPlayer() noexcept;
	~MusicPlayer() noexcept;

	void load(Text16 directory) noexcept;
	void start() noexcept;
	void stop() noexcept;
	void next() noexcept;
	void draw() noexcept;

private:

	void _playNext() noexcept;
	pcstr16 _getNextMusicPath() noexcept;

	// playing
	atomic<MFMixControl*> m_music;
	atomic<pcstr16> m_currentPlaying;
	AText16 m_buffer;

	Array<pcstr16> m_played;
	Array<pcstr16> m_notPlayed;
	size_t m_playedIndex;

	// anim state
	double m_animTick;
	enum class State {
		None,
		Starting,
		Ending,
		Playing,
	};
	State m_state;
};

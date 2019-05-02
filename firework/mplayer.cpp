#include "stdafx.h"
#include "mplayer.h"
#include "drawcontext.h"
#include "res.h"
#include "mfmix.h"

#include <KRUtil/fs/file.h>
#include <KRUtil/fs/path.h>


namespace
{
	void bezierArc(vec2 * dest, float fromAngle, float toAngle) noexcept
	{
		vec2 from = vec2::direction_z(fromAngle);
		vec2 to = vec2::direction_z(toAngle);

		float cp = tanf((toAngle - fromAngle) / 4) * (4.f/3.f);

		*dest++ = {0.f, 0.f};
		*dest++ = from;
		*dest++ = from + from.rotate_xy() * cp;
		*dest++ = to + to.rotate_yx() * cp;
		*dest = to;
	}
}

MusicPlayer::MusicPlayer() noexcept
	:m_currentPlaying(nullptr)
	, m_state(State::None), m_music(nullptr)
{
	m_playedIndex = 0;
}
MusicPlayer::~MusicPlayer() noexcept
{
	stop();
}

void MusicPlayer::load(Text16 directory) noexcept
{
	m_buffer = nullptr;
	m_notPlayed = nullptr;
	m_played = nullptr;
	m_buffer.reserve(1024);
	m_notPlayed.reserve(512);
	
	size_t offset = 0;
	DirectoryScanner scan;
	scan.scan(directory, [&](Text16 path) {
		Text16 ext = path16.extname(path);
		if (ext == u".mp3" || ext == u".ogg" || ext == u".wav" || ext == u".mid")
		{
			m_buffer << path << u'\0';
			m_notPlayed.push((pcstr16)offset);
			offset = m_buffer.size();
		}
	});

	m_buffer.shrink();

	pcstr16 start = m_buffer.data();
	for (pcstr16& path : m_notPlayed)
	{
		path = (size_t)path + start;
	}

	if (m_notPlayed.size() >= 2)
	{
		while (m_notPlayed.size() > m_played.size())
		{
			m_played.push(m_notPlayed.pickRandom());
		}
	}

	m_notPlayed.shrink();
	m_played.shrink();
}
void MusicPlayer::start() noexcept
{
	if (m_music != nullptr) return;
	if (m_notPlayed.empty()) return;

	_playNext();
	m_animTick = g_dc.now;
	m_state = State::Starting;
	g_mix.enableSpectrum();
}
void MusicPlayer::stop() noexcept
{
	MFMixControl* music;
	if ((music = m_music.exchange(nullptr)) == nullptr) return;
	music->stop(true);
	music->Release();

	m_animTick = g_dc.now;
	m_state = State::Ending;
}
void MusicPlayer::next() noexcept
{
	MFMixControl * music;
	if ((music = m_music.exchange(nullptr)) == nullptr) return;
	music->stop(true);
	music->Release();
	_playNext();
}
void MusicPlayer::draw() noexcept
{
	Text16 fileName = nullptr;
	{
		pcstr16 playingFilePath = m_currentPlaying;
		if (playingFilePath != nullptr)
		{
			fileName = (Text16)playingFilePath;
			Text16 last = fileName.find_ry(u"\\/");
			if (last != nullptr) fileName = last + 1;
			Text16 ext = fileName.find_r(u'.');
			if (ext != nullptr) fileName.cut_self(ext);
		}
	}

	float t = 0.f;
	constexpr double PLAYING_TIME = 0.5;
	switch (m_state)
	{
		double it;
	case State::Starting:
		it = g_dc.now - m_animTick;
		if (it >= PLAYING_TIME)
		{
			t = 1.f;
			m_state = State::Playing;
		}
		else
		{
			t = (float)(it / PLAYING_TIME);
		}
		break;
	case State::Ending:
		it = g_dc.now - m_animTick;
		if (it >= PLAYING_TIME)
		{
			t = 0.f;
			m_state = State::None;
		}
		else
		{
			t = (float)(1.0 - it / PLAYING_TIME);
		}
		break;
	case State::None:
		t = 0.f;
		g_mix.disableSpectrum();
		break;
	case State::Playing:
		t = 1.f;
		break;
	}
	t = 1.f - t;
	float it = t * t;
	t = 1.f - it;

	constexpr float MIN_WIDTH = 50.f;
	constexpr float HEIGHT = 40.f;

	frect textRect;
	textRect.bottom = (float)g_dc.height - 2.f;
	textRect.top = textRect.bottom - HEIGHT;

	float textWidth = fileName != nullptr ? g_dc->measureText(fileName, g_res->superNameFont) : 0.f;
	textRect.right = textWidth;
	textRect.right += 60.f;
	if (textRect.right < MIN_WIDTH) textRect.right = MIN_WIDTH;
	
	textRect.left = 2.f;

	float discX = (textRect.right + textRect.left - 21.f) * t + (-20.f) * it;
	float textX = discX - 28.f - textWidth;

	textRect.right = (textRect.right - MIN_WIDTH) * t + MIN_WIDTH;
	textRect.right += textRect.left;

	g_mix.drawSpectrum(t * g_dc.width - g_dc.width, textRect.top - 10.f, (float)g_dc.width, -0.000012f);

	auto _saved = g_dc->save();

	if (m_state != State::None && fileName != nullptr)
	{
		d2d::Path path;
		path.create();
		{
			d2d::PathMake make = path.open();
			make.text(fileName, g_res->superNameFont);
			make.close();
		}
		g_dc->setTransform(mat2p::translate(textX, textRect.bottom - 10.f));
		g_dc->stroke(path, g_res->black, 2.f);
		g_dc->fill(path, g_res->white);
		path.remove();

		g_dc->setTransform(mat2p::scale(15.f, 8.f) * mat2p::translate(discX, (textRect.top + textRect.bottom)*0.5f));
		path.create();

		{
			float axis = (float)fmod(g_dc.now / 0.2, math::taud);
			d2d::PathMake make = path.open();
			for (int i = 0; i < 2; i++)
			{
				vec2 arc[5];
				bezierArc(arc, axis, axis + math::tau/6);
				axis += math::pi;

				for (vec2& v : arc)
				{
					float z = (3.f - v.y) / 3.f;
					float shift = v.y * 0.1f;
					v.x /= z;
					v.x += shift;
					v.y /= z;
				}

				make.begin(arc[0]);
				make.lineTo(arc[1]);
				make.bezierTo({ arc[2], arc[3], arc[4] });
				make.end(true);
			}
			make.close();
		}
		g_dc->stroke(path, g_res->black, 0.2f);
		g_dc->fill(path, g_res->white);
	}

	if (textRect.contains(g_dc.mouse))
	{
		if (g_dc.mousePress)
		{
			if (m_state == State::None)
			{
				start();
			}
			else if (m_state == State::Playing)
			{
				stop();
			}
		}
		g_dc->setTransform(mat2p::identity());
		g_dc->strokeRect(textRect, g_res->red);
		if (m_state == State::None)
		{
			g_dc->setTransform(mat2p::translate(textRect.left + 14.f, (textRect.top + textRect.bottom)/2.f));

			d2d::Path path;
			path.create();
			{
				d2d::PathMake make = path.open();
				make.begin({ 0.f, -7.f });
				make.lineTo({ 13.f, 0.f });
				make.lineTo({ 0.f, 7.f });
				make.end(true);
				make.close();
			}
			g_dc->stroke(path, g_res->red, 2.f);
		}
	}
}

void MusicPlayer::_playNext() noexcept
{
	pcstr16 path = _getNextMusicPath();
	pcstr16 compare_path = nullptr;
	m_currentPlaying = path;

	class Control :public MFMixControl
	{
	public:
		MusicPlayer* player;
	};

	Control* music = _new Control;
	music->AddRef();
	music->player = this;
	g_mix.play(path, music);
	music->onend = [](MFMixControl* param, bool manualStopped) {
		Control* music = static_cast<Control*>(param);
		if (!manualStopped)
		{
			music->player->next();
		}
	};
	MFMixControl* old = m_music.exchange(music);
	if (old)
	{
		old->stop(true);
		old->Release();
	}
}
pcstr16 MusicPlayer::_getNextMusicPath() noexcept
{
	pcstr16& picked = m_notPlayed[g_random.get(m_notPlayed.size())];
	pcstr16 out = picked;
	if (!m_played.empty())
	{
		m_playedIndex = (m_playedIndex + 1) % m_played.size();
		pcstr16 & old = m_played[m_playedIndex];
		pcstr16 tmp = old;
		old = picked;
		picked = tmp;
	}
	return out;
}

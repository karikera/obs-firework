#include "stdafx.h"
#include "mfmix.h"

#include "drawcontext.h"
#include "res.h"
#include <KRSound/mf.h>

using namespace mfmix;

MFMix g_mix;
static const WaveFormat WAVE_FORMAT = {
	1,
	CHANNELS,
	SAMPLES_PER_SEC,
	SAMPLES_PER_SEC * SAMPLE_STRIDE,
	SAMPLE_STRIDE,
	BITS_PER_SAMPLE,
	sizeof(WaveFormat)
};

constexpr size_t SAMPLE_MAX = 1 << (sizeof(sample_t) * 8 - 1);
constexpr size_t SAMPLE_LIMIT = (1 << (sizeof(sample_t) * 8 - 1)) + 1;
static_assert(BITS_PER_SAMPLE == sizeof(sample_t) * 8, "Sample size unmatch");

class MFMixMusic :public Node<MFMixMusic, true>
{
public:
	MFMixMusic(pcstr16 path, MFMixControl * control) noexcept;
	~MFMixMusic() noexcept;
	bool done() noexcept;
	void read(float* dest) noexcept;
	void load() noexcept;

private:
	WaveFormat m_format;
	MFSourceReader m_reader;
	MFMixControl* m_control;

	ABuffer m_data;
	ABuffer m_data_next;
	float m_position;
	bool m_hasNext;
};

SoundSpectrumStream::SoundSpectrumStream() noexcept
{
	m_needed = 0;
	memset(m_drawing, 0, sizeof(m_drawing));
}
SoundSpectrumStream::~SoundSpectrumStream() noexcept
{
}
void SoundSpectrumStream::put(double time, View<float> data) noexcept
{
	m_fftw.create_r2c_1d(SAMPLING_SIZE);
	m_fftw.zero();
	m_needed = SAMPLES_NEXT;

	uint32_t size = intact<uint32_t>((data.sizeBytes() + sizeof(float)) / SAMPLE_STRIDE);
	uint32_t specCount = (size + SAMPLES_NEXT - m_needed) / SAMPLES_NEXT;

	if (size < m_needed)
	{
		m_fftw.put(data.data(), SAMPLE_STRIDE, SAMPLING_SIZE - m_needed, size);
		time += (double)size / SAMPLES_PER_SEC;
		m_needed -= size;
	}
	else
	{
		m_fftw.put(data.data(), SAMPLE_STRIDE, SAMPLING_SIZE - m_needed, m_needed);

		time += (double)m_needed / SAMPLES_PER_SEC;
		_process(time);

		data += m_needed * SAMPLE_STRIDE;
		uint32_t nextBytes = SAMPLES_NEXT * SAMPLE_STRIDE;
		while (data.sizeBytes() >= nextBytes)
		{
			m_fftw.put(data.data(), SAMPLE_STRIDE, SAMPLING_SIZE - SAMPLES_NEXT, SAMPLES_NEXT);
			time += (double)SAMPLES_NEXT / SAMPLES_PER_SEC;
			_process(time);

			data = data.offsetBytes(nextBytes);
		}

		uint32_t left = intact<uint32_t>((data.sizeBytes() + sizeof(float)) / SAMPLE_STRIDE);
		if (!data.empty())
		{
			m_fftw.put(data.data(), SAMPLE_STRIDE, SAMPLING_SIZE - SAMPLES_NEXT, left);
			time += (double)left / SAMPLES_PER_SEC;
			_process(time);
		}
		m_needed = SAMPLES_NEXT - left;
	}
}
void SoundSpectrumStream::readDraw() noexcept
{
	m_queueLock.enter();
	while (!m_queue.empty())
	{
		double curtime;
		m_queue.peek(&curtime, sizeof(double));
		if (curtime >= g_dc.now) break;
		m_queue.skip(sizeof(double));
		m_queue.read(&m_drawing, sizeof(m_drawing));
	}
	m_queueLock.leave();
}
void SoundSpectrumStream::addToPath(float x, float y, float width, float scale, d2d::PathMake& make) noexcept
{
	View<float> drawing = m_drawing;
	auto readY = [&]() {
		return drawing.read()* scale + y;
	};
	float xnext = width / (FREQ_COUNT - 1);

	vec2 prev = { x, readY() };
	vec2 now;
	x += xnext;
	make.begin(prev);
	while (!drawing.empty())
	{
		now = { x, readY() };
		make.quadraticTo(prev, (prev + now) * 0.5f);
		prev = now;

		x += xnext;
	}
	make.lineTo(now);
	make.end(false);
}
d2d::Path SoundSpectrumStream::makePath(float x, float y, float width, float scale) noexcept
{
	d2d::Path path;
	path.create();
	{
		d2d::PathMake make = path.open();
		addToPath(x, y, width, scale, make);
		make.close();
	}
	return path;
}

void SoundSpectrumStream::_process(double time) noexcept
{
	const complex* data = m_fftw.execute();
	const complex* data_end = data + SAMPLING_SIZE / 2 + 1;
	//data += 3;
	//data_end -= 1100;

	float dest[FREQ_COUNT];
	memset(dest, 0, sizeof(dest));
	float i = 0.f;
	float maxfreq = logf((float)SAMPLING_SIZE) * 0.9f;
	float prevY = 0;
	int prevX = 0;

	data++;
	dest[0] = prevY = data->length();
	i++;

	for (; data != data_end; data++)
	{
		float y = data->length();
		int x = math::clamp(0, (int)(logf(i++) / maxfreq * (float)FREQ_COUNT), FREQ_COUNT - 1);

		if (prevX <= x)
		{
			float t = 0.f;
			float td = math::pi / (x - prevX);
			for (; prevX <= x; prevX++)
			{
				dest[prevX] = (y - prevY) * (1 - cos(t)) * 0.5f + prevY;
				t += td;
			}
			prevY = 0.f;
		}
		prevY += y;
	}

	{
		m_queueLock.enter();
		m_queue.write(&time, sizeof(double));
		m_queue.write(dest, sizeof(dest));
		m_queueLock.leave();
	}

	m_fftw.shift(SAMPLES_NEXT);
}


MFMixControl::MFMixControl() noexcept
{
	m_stopped = false;
}
MFMixControl::~MFMixControl() noexcept
{
}
void MFMixControl::stop(bool manual) noexcept
{
	bool compare = false;
	if (!m_stopped.compare_exchange_strong(compare, true)) return;
	onend(this, manual);
}
bool MFMixControl::isStopped() noexcept
{
	return m_stopped;
}

MFMix::MFMix() noexcept
{
	m_audioThread = nullptr;
	m_getTime = nullptr;
}
MFMix::~MFMix() noexcept
{
	remove();
}
void MFMix::create() noexcept
{
	if (m_audioThread != nullptr) return;
	m_spectrum = nullptr;
	m_quit = false;
	m_audioThread = ThreadHandle::create<MFMix, & MFMix::_audioThread>(this);
}
void MFMix::remove() noexcept
{
	if (m_audioThread == nullptr) return;

	m_quit = true;
	m_requestSignal.set();
	m_audioThread->join();
	m_audioThread = nullptr;
	for (Request& request : m_requests)
	{
		request.control->Release();
	}

	m_spectrum = nullptr;
}
void MFMix::setCallback(double (*getTime)(), Callback callback) noexcept
{
	m_callback = callback;
	m_getTime = getTime;
}
MFMixControl* MFMix::play(pcstr16 path) noexcept
{
	MFMixControl* control = _new MFMixControl;
	play(path, control);
	return control;
}
void MFMix::play(pcstr16 path, MFMixControl* control) noexcept
{
	control->AddRef();

	m_requestLock.enter();
	m_requests.push({ path, control });
	m_requestLock.leave();
	m_request = true;
	m_requestSignal.set();
}
void MFMix::enableSpectrum() noexcept
{
	if (m_spectrum == nullptr)
	{
		m_spectrumLock.enter();
		if (m_spectrum == nullptr)
		{
			m_spectrum = _new SoundSpectrumStream[2];
		}
		m_spectrumLock.leave();
	}
}
void MFMix::disableSpectrum() noexcept
{
	m_spectrumLock.enter();
	SoundSpectrumStream * spectrum = m_spectrum.exchange(nullptr);
	m_spectrumLock.leave();
	delete [] spectrum;
}
void MFMix::drawSpectrum(float x, float y, float width, float scale) noexcept
{
	if (m_spectrum != nullptr)
	{
		m_spectrumLock.enter();
		m_spectrum[0].readDraw();
		m_spectrum[1].readDraw();
		m_spectrumLock.leave();

		d2d::Path pathL = m_spectrum[0].makePath(x, y, width, scale);
		d2d::Path pathR = m_spectrum[1].makePath(x, y, width, scale);

		g_dc->stroke(pathL, g_res->black, 5);
		g_dc->stroke(pathR, g_res->black, 5);
		g_dc->stroke(pathL, g_res->red, 3);
		g_dc->stroke(pathR, g_res->white, 3);
	}
}
int MFMix::_audioThread() noexcept
{
	try
	{
		double playTo = m_getTime();

		LinkedList<MFMixMusic> readers;

		float buffers[CHUNK_SIZE * CHANNELS];
		sample_t samples[CHUNK_SIZE * CHANNELS];


		for (;;)
		{
			memset(buffers, 0, sizeof(buffers));
			for (MFMixMusic& music : readers)
			{
				music.read(buffers);
			}

			for (float &v : buffers)
			{
				constexpr float minv = (float)minof(sample_t);
				constexpr float maxv = (float)maxof(sample_t);

				v = math::clamp(minv, atanf(v) * (SAMPLE_LIMIT / (math::pi / 2.f)), maxv);
			}

			if (m_spectrum != nullptr)
			{
				m_spectrumLock.enter();
				if (m_spectrum != nullptr)
				{
					View<float> sbuf = buffers;
					m_spectrum[0].put(playTo, sbuf);
					m_spectrum[1].put(playTo, sbuf + 1);
				}
				m_spectrumLock.leave();
			}

			for (MFMixMusic& music : readers)
			{
				music.load();
			}
			readers.removeMatchAll([&](MFMixMusic * music) { return music->done(); });

			duration left = duration::fromRealTime((float)(playTo - m_getTime()));
			if (left > 0_ms && m_requestSignal.wait(left))
			{
				if (m_quit) break;
				if (m_request)
				{
					m_request = false;
					m_requestLock.enter();
					while (!m_requests.empty())
					{
						Request request = m_requests.popGet();
						MFMixMusic* music = _new MFMixMusic(request.path, request.control);
						readers.attach(music);
					}
					m_requestLock.leave();
				}
			}

			{
				sample_t* dest = samples;
				for (float v : buffers)
				{
					*dest++ = (sample_t)v;
				}
			}
			m_callback(&WAVE_FORMAT, Buffer(samples, sizeof(samples)), playTo);
			playTo += CHUNK_DURATION;
		}
	}
	catch (ThrowAbort&)
	{
	}
	catch (ErrorCode & err)
	{
#ifndef NDEBUG
		udout << err.getMessage<char16>() << endl;
		udout.flush();
		debug();
#endif
	}
	return 0;
}

MFMixMusic::MFMixMusic(pcstr16 path, MFMixControl* control) noexcept
	:m_reader(MFSourceReader::load(path, &m_format)),
	m_control(control)
{
	m_hasNext = true;
	m_position = 0;
}
MFMixMusic::~MFMixMusic() noexcept
{
	m_control->Release();
}
bool MFMixMusic::done() noexcept
{
	return m_control->isStopped();
}
void MFMixMusic::read(float* dest) noexcept
{
	if (m_format.channels <= 0) return;
	_assert(m_format.bitsPerSample == BITS_PER_SAMPLE);

	float mapping = (float)m_format.samplesPerSec / SAMPLES_PER_SEC;
	float* dest_end = dest + CHUNK_SIZE * CHANNELS;
	float data_size = (float)(m_data.size() / m_format.blockAlign);

	goto _start;
_shiftPosition:
	m_position -= data_size;
	m_data.clear();
	if (m_data_next.empty())
	{
		data_size = 0.f;
		return;
	}
	{
		ABuffer tmp = move(m_data);
		m_data = move(m_data_next);
		m_data_next = move(tmp);
	}
	data_size = (float)(m_data.size() / m_format.blockAlign);
	
_start:
	if (m_format.channels == 1)
	{
		while (dest != dest_end)
		{
			if (m_position >= data_size) goto _shiftPosition;

			sample_t* psample = (sample_t*)m_data.data() + (int)(m_position);
			sample_t sample = *psample;
			m_position += mapping;
			
			*dest += tanf(sample * (math::pi / 2.f / SAMPLE_LIMIT));
			dest++;
			*dest += tanf(sample * (math::pi / 2.f / SAMPLE_LIMIT));
			dest++;
		}
	}
	else 
	{
		while (dest != dest_end)
		{
			if (m_position >= data_size) goto _shiftPosition;

			sample_t* psample = (sample_t*)m_data.data() + (int)(m_position) * 2;
			m_position += mapping;

			*dest += tanf(*psample * (math::pi / 2.f / SAMPLE_LIMIT));
			psample++;
			dest++;
			*dest += tanf(*psample * (math::pi / 2.f / SAMPLE_LIMIT));
			dest++;
		}
	}
}
void MFMixMusic::load() noexcept
{
	if (m_hasNext)
	{
		while (m_data_next.size() < SAMPLES_PER_SEC * m_format.blockAlign)
		{
			MFMediaBuffer buffer = m_reader.read();
			if (buffer != nullptr)
			{
				Buffer locked;
				buffer.lock(&locked);
				m_data_next << locked;
				buffer.unlock();
			}
			else
			{
				m_hasNext = false;
				goto _done;
			}
		}
	}
	else
	{
	_done:
		if (m_data.empty() && m_data_next.empty())
		{
			m_control->stop(false);
		}
	}
}

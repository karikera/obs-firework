#pragma once

#include "drawobject.h"
#include <KR3/mt/criticalsection.h>
#include <KRUtil/bufferqueue.h>
#include <KRSound/fft.h>

namespace mfmix
{
	using sample_t = short;
	static constexpr int MIN_FREQ = 20;
	static constexpr int MAX_FREQ = 48000;
	static constexpr int FREQ_COUNT = 100;
	static constexpr int CHANNELS = 2;
	static constexpr int SAMPLES_PER_SEC = 48000;
	static constexpr int BITS_PER_SAMPLE = 16;
	static constexpr double CHUNK_DURATION = 0.5;
	static constexpr int FPS = 30;
	static constexpr int CHUNK_SIZE = (int)(SAMPLES_PER_SEC * CHUNK_DURATION);
	static constexpr int SAMPLE_STRIDE = CHANNELS * sizeof(sample_t);
	static constexpr int SAMPLING_SIZE = SAMPLES_PER_SEC / MIN_FREQ;
	static constexpr int SAMPLES_NEXT = SAMPLES_PER_SEC / FPS;
}

class SoundSpectrumStream
{
public:	
	SoundSpectrumStream() noexcept;
	~SoundSpectrumStream() noexcept;
	void put(double time, View<float> data) noexcept;
	void readDraw() noexcept;
	void addToPath(float x, float y, float width, float scale, d2d::PathMake& make) noexcept;
	d2d::Path makePath(float x, float y, float width, float scale) noexcept;

private:
	void _process(double time) noexcept;

	uint32_t m_needed;
	uint32_t m_previousPos;

	CriticalSection m_queueLock;
	BufferQueue m_queue;
	kr::sound::FFTW<float, complex> m_fftw;
	float m_drawing[mfmix::FREQ_COUNT];
};

class MFMixControl:public AtomicReferencable<MFMixControl>
{
public:
	MFMixControl() noexcept;
	~MFMixControl() noexcept;
	void stop(bool manual) noexcept;
	bool isStopped() noexcept;

	void (*onend)(MFMixControl*, bool);

private:
	atomic<bool> m_stopped;

};

class MFMix
{
public:
	using Callback = Lambda<sizeof(void*), void(const WaveFormat* format, Buffer data, double time)>;

	MFMix() noexcept;
	~MFMix() noexcept;
	void create() noexcept;
	void remove() noexcept;
	void setCallback(double (*getTime)(), Callback callback) noexcept;
	MFMixControl* play(pcstr16 path) noexcept;
	void play(pcstr16 path, MFMixControl* control) noexcept;
	void enableSpectrum() noexcept;
	void disableSpectrum() noexcept;
	void drawSpectrum(float x, float y, float width, float scale) noexcept;

private:
	int _audioThread() noexcept;
	struct Request
	{
		pcstr16 path;
		MFMixControl * control;
	};

	// thread
	ThreadHandle* m_audioThread;

	// callback
	Callback m_callback;
	double (*m_getTime)();

	// request
	atomic<bool> m_quit;
	atomic<bool> m_request;
	CriticalSection m_requestLock;
	Array<Request> m_requests;
	Cond m_requestSignal;

	// sepctrum
	atomic<SoundSpectrumStream*> m_spectrum;
	CriticalSection m_spectrumLock;

};

extern MFMix g_mix;

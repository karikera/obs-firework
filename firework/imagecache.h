#pragma once

#include <KR3/util/callable.h>
#include <KRMessage/promise.h>
#include <KRApp/dx/d2d.h>
#include <KRApp/dx/wic.h>
#include <KR3/data/linkedlist.h>

using namespace kr;

class CachedImage: public Referencable<CachedImage>, public Node<CachedImage, true>
{
public:
	CachedImage(AText url) noexcept;
	~CachedImage() noexcept;
	d2d::Bitmap image() noexcept;
	d2d::Bitmap image(size_t index) noexcept;
	size_t getFrameCount() noexcept;
	void onLoad(CallablePtrT<void(bool)> onload) noexcept;
	static Keep<CachedImage> load(AText url) noexcept;
	static void loadImage() noexcept;
	static void unloadAll() noexcept;
	
private:
	void _requestThread() noexcept;
	void _load() noexcept;

	AText m_url;
	AText16 m_file;
	d2d::Bitmap m_image;
	Array<d2d::Bitmap> m_frames;
	WICBitmapDecoder m_decoder;
	CallableListT<void(bool)> m_onLoad;
	enum class Status
	{
		Waiting,
		Downloading,
		Reading,
		Loaded,
		Error,
	};
	Status m_status;
};

class ImageKeeper
{
public:
	ImageKeeper() noexcept;
	ImageKeeper(AText url) noexcept;
	~ImageKeeper() noexcept;
	bool isLoaded() noexcept;
	void load(AText url) noexcept;
	void set(Keep<CachedImage> image) noexcept;
	void resetFrameTime() noexcept;
	vec2 getSize() noexcept;
	d2d::Bitmap getImage() noexcept;
	void draw(d2d::RenderTarget *target) noexcept;
	void draw(d2d::RenderTarget *target, vec2 pos) noexcept;
	void draw(d2d::RenderTarget *target, const frect &rect) noexcept;
	void onLoad(CallablePtrT<void(bool)> onload) noexcept;

private:
	CachedImage * m_image;
	timepoint m_startTime;
};
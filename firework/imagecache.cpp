#include "stdafx.h"
#include "imagecache.h"
#include "drawcontext.h"
#include "util.h"

#include <KR3/data/map.h>
#include <KRUtil/fs/file.h>
#include <KRUtil/fetch.h>
#include <KRMessage/pool.h>

#include <Shlwapi.h>
#include <atlbase.h>

namespace
{
	ReferenceMap<Text, CachedImage*> s_map;
	Chain<CachedImage> s_loadQueue;
}

CachedImage::CachedImage(AText url) noexcept
	:m_url(move(url))
{
	m_file << u"temp/" << utf8ToUtf16(m_url) << nullterm;
	auto urlname = m_file.subarr(5);
	urlname.change(u'/', u'_');
	urlname.change(u':', u'_');

	m_status = Status::Waiting;
}
CachedImage::~CachedImage() noexcept
{
	s_map.erase(m_url);
}
d2d::Bitmap CachedImage::image() noexcept
{
	return m_image;
}
d2d::Bitmap CachedImage::image(size_t index) noexcept
{
	index %= getFrameCount();
	if (index == 0) return m_image;
	index--;
	if (index >= m_frames.size()) return m_frames.back();
	return m_frames[index];
}
size_t CachedImage::getFrameCount() noexcept
{
	return m_frames.size() + 1;
}
void CachedImage::onLoad(CallablePtrT<void(bool)> onload) noexcept
{
	if (m_status > Status::Reading)
	{
		onload->call(m_status == Status::Loaded ? true : false);
		delete onload;
	}
	else
	{
		m_onLoad.add(onload);
	}
}
Keep<CachedImage> CachedImage::load(AText url) noexcept
{
	if (url.empty()) return nullptr;

	url.c_str();
	auto res = s_map.insert(url, nullptr);
	if (res.second)
	{
		CachedImage * img = _new CachedImage(move(url));
		res.first->second = img;
		img->_requestThread();
		return img;
	}
	return res.first->second;
}
void CachedImage::loadImage() noexcept
{
	if (!g_dc.isCreated()) return;
	if (s_loadQueue.empty()) return;
	auto * image = s_loadQueue.detachFirst();
	image->_load();
	image->Release();
}
void CachedImage::unloadAll() noexcept
{
	for (auto & pair : s_map)
	{
		CachedImage * image = pair.second;
		if (image->m_status == Status::Loaded)
		{
			image->m_image = nullptr;
			image->m_frames = nullptr;
			image->m_decoder = nullptr;
			image->m_onLoad.clear();
			image->_requestThread();
		}
	}
}

void CachedImage::_requestThread() noexcept
{
	m_status = Status::Downloading;
	AddRef();
	threading([this] {
		try
		{
			if (File::exists(m_file.data()))
			{
				Must<File> file = File::open(m_file.data());
				if (file->size() != 0)
				{
					goto _decode;
				}
				File::remove(m_file.data());
			}
			{
				HttpRequest request;
				request.fetchAsFileSync(m_url.data(), m_file.data());
			}
		_decode:
			m_decoder.create(m_file.data());
		}
		catch (...)
		{
			File::remove(m_file.data());
			throw;
		}
	})->then([this] {
		if (getReferenceCount() == 1)
		{
			Release();
			return;
		}
		m_status = Status::Reading;
		s_loadQueue.attach(this);
	})->katch([this](Exception&) {
		m_status = Status::Error;
		m_onLoad.fire(false);
		m_onLoad.clear();
		Release();
	});
}
void CachedImage::_load() noexcept
{
	try
	{
		if (m_decoder != nullptr)
		{
			m_image = g_dc->loadImage(m_decoder.getFrame(0));

			uint count = m_decoder.getFrameCount();
			m_frames.reserve(count - 1);
			for (uint i = 1; i < count; i++)
			{
				m_frames[i] = g_dc->loadImage(m_decoder.getFrame(i));
			}
			m_decoder = nullptr;
		}
	}
	catch (...)
	{
	}

	//Read from IStream    
	m_status = Status::Loaded;
	m_onLoad.fire(true);
	m_onLoad.clear();
}


ImageKeeper::ImageKeeper() noexcept
{
	m_image = nullptr;
}
ImageKeeper::ImageKeeper(AText url) noexcept
	:ImageKeeper()
{
	load(move(url));
}
ImageKeeper::~ImageKeeper() noexcept
{
	if (m_image)
	{
		m_image->Release();
	}
}

bool ImageKeeper::isLoaded() noexcept
{
	return m_image != nullptr;
}
void ImageKeeper::load(AText url) noexcept
{
	set(CachedImage::load(move(url)));
}
void ImageKeeper::set(Keep<CachedImage> image) noexcept
{
	if (m_image) m_image->Release();
	m_image = image.detach();
	resetFrameTime();
}

void ImageKeeper::resetFrameTime() noexcept
{
	m_startTime = timepoint::now();
}
vec2 ImageKeeper::getSize() noexcept
{
	if (m_image == nullptr) return {1, 1};
	return m_image->image().getSize();
}
d2d::Bitmap ImageKeeper::getImage() noexcept
{
	if (m_image == nullptr) return nullptr;
	uint index = ((timepoint::now() - m_startTime) / 30_s);
	return m_image->image(index);
}
void ImageKeeper::draw(d2d::RenderTarget *target) noexcept
{
	target->drawImage(getImage());
}
void ImageKeeper::draw(d2d::RenderTarget *target, vec2 pos) noexcept
{
	target->drawImage(getImage(), {pos, pos + getSize()});
}
void ImageKeeper::draw(d2d::RenderTarget *target, const frect &rect) noexcept
{
	target->drawImage(getImage(), rect);
}
void ImageKeeper::onLoad(CallablePtrT<void(bool)> onload) noexcept
{
	if (m_image != nullptr)
	{
		m_image->onLoad(move(onload));
	}
	else
	{
		onload->call(false);
	}
}
#include "stdafx.h"
#include "drawcontext.h"

#include <KRUtil/text/codepoint.h>

OffscreenDrawContext g_dc;


OffscreenDrawContext::OffscreenDrawContext() noexcept
{
	now = 0.0;
	delta = 0.f;
	width = 0;
	height = 0;
	mouseIn = false;
}
OffscreenDrawContext::~OffscreenDrawContext() noexcept
{
	remove();
}

bool OffscreenDrawContext::isCreated() noexcept
{
	return m_d2d != nullptr;
}
bool OffscreenDrawContext::create(ID3D11Texture2D* texture, int textureWidth, int textureHeight) noexcept
{
	remove();

	width = textureWidth;
	height = textureHeight;

	try
	{
		m_d2dWindow.create();

		{
			dxgi::Surface surface;
			surface.setFrom(texture);
			m_d2dBitmap = m_d2dWindow.createBitmapFromSurface(surface);
			m_d2d.createFromSurface(surface);
		}
		return true;
	}
	catch (ErrorCode & err)
	{
		udout << err.getMessage<char16>() << endl;
		remove();
		return false;
	}
}

void OffscreenDrawContext::remove() noexcept
{
	m_d2dBitmap.remove();
	m_d2dWindow.remove();
	m_d2d.remove();
}
void OffscreenDrawContext::present() noexcept
{
	if (m_d2dWindow != nullptr)
	{
		m_d2dWindow.beginDraw();
		m_d2dWindow.clear({0.f, 0.f, 0.f, 0.f});
		m_d2dWindow.drawImage(m_d2dBitmap, {0.f, 0.f, (float)width, (float)height});
		m_d2dWindow.endDraw();
	}
}

d2d::DeviceContext * OffscreenDrawContext::dc() noexcept
{
	return &m_d2dWindow;
}
d2d::RenderTarget * OffscreenDrawContext::operator ->() noexcept
{
	return &m_d2d;
}
OffscreenDrawContext::operator d2d::RenderTarget*() noexcept
{
	return &m_d2d;
}

TextBox::TextBox(Text text, d2d::Font font, float widthMax, View<EmoticonInfo> emotes) noexcept
	:m_font(font), m_indentation(0), m_height(0), m_width(widthMax)
{
	m_text.reserve(text.size());
	if (emotes != nullptr)
	{
		m_emotes.reserve(emotes.size());

		CodePoint<char, Charset::Utf8> cp = text;
		auto iter = cp.begin();
		auto iter_end = cp.end();

		size_t cpIndex = 0;

		for (const EmoticonInfo & emote : emotes)
		{
			for (; cpIndex != emote.start; cpIndex++)
			{
				if (iter != iter_end) iter++;
			}
			size_t prevSize = m_text.size();
			m_text << utf8ToUtf16(text.cut(*iter));
			for (; cpIndex != emote.end; cpIndex++)
			{
				if (iter != iter_end) iter++;
			}
			text.setBegin((*iter).end());

			m_text.subarr(prevSize).change(u'\0', u' ');
			size_t index = m_text.size();
			m_text << u'\0';

			Keep<CachedImage> image = CachedImage::load(move(emote.url));
			if (image) image->AddRef();

			m_emotes.push({ index, image, 0.f });
		}
		m_text << utf8ToUtf16(text);
	}
	else
	{
		m_text << utf8ToUtf16(text);
		m_text.change(u'\0', u' ');
	}

}
TextBox::~TextBox() noexcept
{
	for (auto & image : m_emotes)
	{
		image.image->Release();
	}
}
View<TextBox::ImageCharacter> TextBox::emotes() noexcept
{
	return m_emotes;
}
bool TextBox::isCalculated() noexcept
{
	return m_height != 0;
}
void TextBox::calculateTextArea(float indentation) noexcept
{
	m_indentation = indentation;

	float lineHeight = m_font.getHeight();
	float totalHeight = 0;
	float currentHeight = lineHeight;

	float x = indentation;
	auto addWidth = [&](float width, size_t index) {
		float nx = x + width;
		if (nx > m_width)
		{
			m_splits.push({ index, currentHeight });
			totalHeight += currentHeight;
			currentHeight = lineHeight;
			x = width;
		}
		else
		{
			x = nx;
		}
	};

	auto * emotes_iter = m_emotes.begin();

	for (Text16 chr : codePoint<char16>(m_text))
	{
		size_t index = chr - m_text;
		if (*chr == u'\0')
		{
			vec2 size = emotes_iter->image->image().getSize();
			addWidth(size.x, index);
			if (currentHeight < size.y) currentHeight = size.y;
			emotes_iter->x = x - size.x;
			emotes_iter++;
		}
		else
		{
			float width = g_dc->measureText(chr, m_font);
			addWidth(width, index);
		}
	}
	m_splits.push({ m_text.size(), currentHeight });
	m_height = totalHeight + currentHeight;
}
void TextBox::draw(float x, float y, d2d::Brush brush) noexcept
{
	if (!isCalculated()) return;

	auto * emotes_iter = m_emotes.begin();
	auto * emotes_end = m_emotes.end();

	float nx = x + m_indentation;
	size_t from = 0;
	for (Split& split : m_splits)
	{
		while (emotes_iter != emotes_end)
		{
			if (emotes_iter->index >= split.index) break;

			Text16 tx = m_text.subarray(from, emotes_iter->index);
			g_dc->fillText(tx, m_font, { nx, y }, brush);
			nx = x + emotes_iter->x;
			d2d::Bitmap image = emotes_iter->image->image();
			vec2 size = image.getSize();
			g_dc->drawImage(image, {
				nx, y,
				nx + size.x, y + size.y
			});
			nx += size.x;
			from = emotes_iter->index+1;
			emotes_iter++;
		}
		Text16 tx = m_text.subarray(from, split.index);
		g_dc->fillText(tx, m_font, { nx, y }, brush);
		from = split.index;
		nx = x;
		y += split.lineHeight;
	}
	
	Text16 tx = m_text.subarray(from);
	g_dc->fillText(tx, m_font, { nx, y }, brush);
}
float TextBox::getWidth() noexcept
{
	return m_width;
}
float TextBox::getHeight() noexcept
{
	return m_height;
}
float TextBox::getLineHeight(size_t index) noexcept
{
	return m_splits[index].lineHeight;
}

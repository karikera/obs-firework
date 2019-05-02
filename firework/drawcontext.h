#pragma once

#include <KRApp/dx/d3d11.h>
#include <KRApp/dx/d2d.h>
#include <KR3/data/map.h>

#include "imagecache.h"

using namespace kr;

class OffscreenDrawContext
{
public:
	int width;
	int height;
	vec2 mouse;
	bool mouseDown;
	bool mousePress;
	bool mouseRelease;
	bool mouseIn;
	double now;
	float delta;

	OffscreenDrawContext() noexcept;
	~OffscreenDrawContext() noexcept;
	bool isCreated() noexcept;
	bool create(ID3D11Texture2D* texture, int textureWidth, int textureHeight) noexcept;
	void remove() noexcept;
	void present() noexcept;

	d2d::DeviceContext * dc() noexcept;
	d2d::RenderTarget * operator ->() noexcept;
	operator d2d::RenderTarget *() noexcept;

private:
	d2d::RenderTarget m_d2d;
	d2d::Bitmap m_d2dBitmap;
	d2d::DeviceContext m_d2dWindow;
};

struct EmoticonInfo
{
	size_t start;
	size_t end;
	AText url;

	inline size_t getKey() const noexcept
	{
		return start;
	}
};

class TextBox
{
public:
	struct ImageCharacter
	{
		size_t index;
		CachedImage * image;
		float x;
	};

private:
	struct Split
	{
		size_t index;
		float lineHeight;
	};
	AText16 m_text;
	float m_indentation;
	float m_width;
	float m_height;
	Array<Split> m_splits;
	d2d::Font m_font;
	Array<ImageCharacter> m_emotes;

public:
	TextBox(Text text, d2d::Font font, float widthMax, View<EmoticonInfo> emotes) noexcept;
	~TextBox() noexcept;
	View<ImageCharacter> emotes() noexcept;
	bool isCalculated() noexcept;
	void calculateTextArea(float indentation) noexcept;
	void draw(float x, float y, d2d::Brush brush) noexcept;
	float getWidth() noexcept;
	float getHeight() noexcept;
	float getLineHeight(size_t index) noexcept;

};

extern OffscreenDrawContext g_dc;

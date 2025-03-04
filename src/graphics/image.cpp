#include "image.h"
#include "../math/colour.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image_write.h"

using namespace llt;

static void _stbi_write(void *context, void *data, int size)
{
	((Stream *)context)->write((char *)data, size);
}

Image::Image()
	: m_pixels(nullptr)
	, m_width(0)
	, m_height(0)
	, m_channels(0)
	, m_stbiManaged(false)
	, m_format(FORMAT_RGBA8)
{
}

Image::Image(const String &path)
	: Image(path.cstr())
{
}

Image::Image(const char *path)
	: Image()
{
	load(path);
}

Image::Image(int width, int height)
	: m_width(width)
	, m_height(height)
	, m_channels(0)
	, m_stbiManaged(false)
	, m_format(FORMAT_RGBA8)
{
	m_pixels = new Colour[width * height];
}

Image::~Image()
{
	free();
}

void Image::load(const String &path)
{
	load(path.cstr());
}

void Image::load(const char *path)
{
	int w, h, channels;

	if (stbi_is_hdr(path))
	{
		m_pixels = stbi_loadf(path, &w, &h, &channels, 4);
		m_format = FORMAT_RGBAF;
	}
	else
	{
		m_pixels = stbi_load(path, &w, &h, &channels, 4);
		m_format = FORMAT_RGBA8;

		if (!m_pixels)
			LLT_ERROR("Couldn't load image :(");
	}

	this->m_width = w;
	this->m_height = h;
	this->m_channels = channels;

	this->m_stbiManaged = true;
}

void Image::free()
{
	if (!m_pixels) {
		return;
	}

	if (m_stbiManaged) {
		stbi_image_free(m_pixels);
	} else {
		delete[] (Colour *)m_pixels;
	}

	m_pixels = nullptr;
}

void Image::paint(const BrushFn &brush)
{
	paint(RectI(0, 0, m_width, m_height), brush);
}

void Image::paint(const RectI &rect, const BrushFn &brush)
{
	for (int y = 0; y < rect.h; y++)
	{
		for (int x = 0; x < rect.w; x++)
		{
			int px = rect.x + x;
			int py = rect.y + y;

			int idx = (py * m_width) + px;

			((Colour *)m_pixels)[idx] = brush(x, y);
		}
	}
}

Colour Image::getPixelAt(uint32_t x, uint32_t y) const
{
	return ((Colour *)m_pixels)[(y * m_width) + x];
}

void Image::setPixels(const Colour *data)
{
	mem::copy(m_pixels, data, sizeof(Colour) * getPixelCount());
}

void Image::setPixels(uint64_t dstFirst, const Colour *data, uint64_t srcFirst, uint64_t count)
{
	mem::copy((Colour *)m_pixels + sizeof(Colour) * dstFirst, data + sizeof(Colour) * srcFirst, sizeof(Colour) * count);
}

bool Image::saveToPng(const char *file) const
{
	FileStream fs(file, "wb");
	return saveToPng(fs);
}

bool Image::saveToPng(Stream &stream) const
{
	LLT_ASSERT(m_pixels, "Pixel data cannot be null.");
	LLT_ASSERT(m_width > 0 && m_height > 0, "Width and Height must be > 0.");

	stbi_write_force_png_filter = 0;
	stbi_write_png_compression_level = 0;

	if (stbi_write_png_to_func(_stbi_write, &stream, m_width, m_height, 4, m_pixels, m_width * 4) != 0) {
		return true;
	} else {
		LLT_ERROR("stbi_write_png_to_func(...) failed.");
	}

	return false;
}

bool Image::saveToJpg(const char *file, int quality) const
{
	FileStream fs(file, "wb");
	return saveToJpg(fs, quality);
}

bool Image::saveToJpg(Stream &stream, int quality) const
{
	LLT_ASSERT(m_pixels, "Pixel data cannot be null.");
	LLT_ASSERT(m_width > 0 && m_height > 0, "Width and Height must be > 0.");

	if (quality < 1) {
		LLT_LOG("JPG quality value should be between [1, 100].");
		quality = 1;
	} else if (quality > 100) {
		LLT_LOG("JPG quality value should be between [1, 100].");
		quality = 100;
	}

	if (stbi_write_jpg_to_func(_stbi_write, &stream, m_width, m_height, 4, m_pixels, quality) != 0) {
		return true;
	} else {
		LLT_ERROR("stbi_write_jpg_to_func(...) failed.");
	}

	return false;
}

void *Image::getData()
{
	return m_pixels;
}

const void *Image::getData() const
{
	return m_pixels;
}

Image::Format Image::getFormat() const
{
	return m_format;
}

uint32_t Image::getWidth() const
{
	return m_width;
}

uint32_t Image::getHeight() const
{
	return m_height;
}

uint32_t Image::getPixelCount() const
{
	return m_width * m_height;
}

uint64_t Image::getSize() const
{
	uint64_t unit = m_format == FORMAT_RGBA8 ? sizeof(uint8_t) : sizeof(float);
	return m_width * m_height * 4 * unit;
}

int Image::getChannels() const
{
	return m_channels;
}

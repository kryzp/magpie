#include "bitmap.h"

#include "core/platform.h"

#include "math/colour.h"

#include "io/file_stream.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

using namespace mgp;

static void stbiWriteCallback(void *context, void *data, int size)
{
	((Stream *)context)->write((char *)data, size);
}

Bitmap::Bitmap()
	: m_pixels(nullptr)
	, m_width(0)
	, m_height(0)
	, m_channels(0)
	, m_stbiManaged(false)
	, m_format(FORMAT_RGBA8)
{
}

Bitmap::Bitmap(const std::string &path)
	: Bitmap(path.c_str())
{
}

Bitmap::Bitmap(const char *path)
	: Bitmap()
{
	load(path);
}

Bitmap::Bitmap(int width, int height)
	: m_pixels(nullptr)
	, m_format(FORMAT_RGBA8)
	, m_width(width)
	, m_height(height)
	, m_channels(0)
	, m_stbiManaged(false)
{
	m_pixels = new Colour[width * height];
}

Bitmap::~Bitmap()
{
	free();
}

void Bitmap::load(const std::string &path)
{
	load(path.c_str());
}

void Bitmap::load(const char *path)
{
	int w, h, channels;

	if (stbi_is_hdr(path))
	{
		m_pixels = stbi_loadf(path, &w, &h, &channels, 4);
		m_format = FORMAT_RGBAF;

		if (!m_pixels)
			MGP_ERROR("Couldn't load Bitmap HDR :(");
	}
	else
	{
		m_pixels = stbi_load(path, &w, &h, &channels, 4);
		m_format = FORMAT_RGBA8;

		if (!m_pixels)
			MGP_ERROR("Couldn't load Bitmap LDR :(");
	}

	this->m_width = w;
	this->m_height = h;
	this->m_channels = channels;

	this->m_stbiManaged = true;
}

void Bitmap::free()
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

void Bitmap::paint(const std::function<Colour(uint32_t, uint32_t)> &brush)
{
	paint(RectI(0, 0, m_width, m_height), brush);
}

void Bitmap::paint(const RectI &rect, const std::function<Colour(uint32_t, uint32_t)> &brush)
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

Colour Bitmap::getPixelAt(uint32_t x, uint32_t y) const
{
	return ((Colour *)m_pixels)[(y * m_width) + x];
}

void Bitmap::setPixels(const Colour *data)
{
	mem::copy(m_pixels, data, sizeof(Colour) * getPixelCount());
}

void Bitmap::setPixels(uint64_t dstFirst, const Colour *data, uint64_t srcFirst, uint64_t count)
{
	mem::copy((Colour *)m_pixels + sizeof(Colour) * dstFirst, data + sizeof(Colour) * srcFirst, sizeof(Colour) * count);
}

bool Bitmap::saveToPng(const Platform *platform, const char *file) const
{
	FileStream fs(platform, file, "wb");
	return saveToPng(fs);
}

bool Bitmap::saveToPng(Stream &stream) const
{
	MGP_ASSERT(m_pixels, "Pixel data cannot be null.");
	MGP_ASSERT(m_width > 0 && m_height > 0, "Width and Height must be > 0.");

	stbi_write_force_png_filter = 0;
	stbi_write_png_compression_level = 0;

	if (stbi_write_png_to_func(stbiWriteCallback, &stream, m_width, m_height, 4, m_pixels, m_width * 4) != 0) {
		return true;
	} else {
		MGP_ERROR("stbi_write_png_to_func(...) failed.");
	}

	return false;
}

bool Bitmap::saveToJpg(const Platform *platform, const char *file, int quality) const
{
	FileStream fs(platform, file, "wb");
	return saveToJpg(fs, quality);
}

bool Bitmap::saveToJpg(Stream &stream, int quality) const
{
	MGP_ASSERT(m_pixels, "Pixel data cannot be null.");
	MGP_ASSERT(m_width > 0 && m_height > 0, "Width and Height must be > 0.");

	if (quality < 1) {
		MGP_LOG("JPG quality value should be between [1, 100].");
		quality = 1;
	} else if (quality > 100) {
		MGP_LOG("JPG quality value should be between [1, 100].");
		quality = 100;
	}

	if (stbi_write_jpg_to_func(stbiWriteCallback, &stream, m_width, m_height, 4, m_pixels, quality) != 0) {
		return true;
	} else {
		MGP_ERROR("stbi_write_jpg_to_func(...) failed.");
	}

	return false;
}

void *Bitmap::getData()
{
	return m_pixels;
}

const void *Bitmap::getData() const
{
	return m_pixels;
}

Bitmap::Format Bitmap::getFormat() const
{
	return m_format;
}

uint32_t Bitmap::getWidth() const
{
	return m_width;
}

uint32_t Bitmap::getHeight() const
{
	return m_height;
}

uint32_t Bitmap::getPixelCount() const
{
	return m_width * m_height;
}

uint64_t Bitmap::getMemorySize() const
{
	uint64_t unit = m_format == FORMAT_RGBA8 ? sizeof(uint8_t) : sizeof(float);
	return m_width * m_height * 4 * unit;
}

int Bitmap::getChannelCount() const
{
	return m_channels;
}

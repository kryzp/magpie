#include "image.h"
#include "colour.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image_write.h"

using namespace llt;

static void _stbi_write(void* context, void* data, int size)
{
	((Stream*)context)->write((char*)data, size);
}

Image::Image()
	: m_pixels(nullptr)
	, m_width(0)
	, m_height(0)
	, m_nrChannels(0)
	, m_stbiManaged(false)
{
}

Image::Image(const char* path)
	: Image()
{
	load(path);
}

Image::Image(int width, int height)
	: m_width(width)
	, m_height(height)
	, m_nrChannels(0)
	, m_stbiManaged(false)
{
	this->m_pixels = new Colour[width * height];
}

Image::~Image()
{
	free();
}

void Image::load(const char* path)
{
	this->m_stbiManaged = true;

	int w, h, nrc;
	this->m_pixels = (Colour*)stbi_load(path, &w, &h, &nrc, 4);
	this->m_width = w;
	this->m_height = h;
	this->m_nrChannels = nrc;
}

void Image::free()
{
	if (!m_pixels) {
		return;
	}

	if (m_stbiManaged) {
		stbi_image_free(m_pixels);
	} else {
		delete[] m_pixels;
	}

	m_pixels = nullptr;
}

void Image::paint(const BrushFn& brush)
{
	paint(RectI(0, 0, m_width, m_height), brush);
}

void Image::paint(const RectI& rect, const BrushFn& brush)
{
	for (int y = 0; y < rect.h; y++)
	{
		for (int x = 0; x < rect.w; x++)
		{
			int px = rect.x + x;
			int py = rect.y + y;
			int idx = (py * m_width) + px;

			m_pixels[idx] = brush(x, y);
		}
	}
}

void Image::setPixels(const Colour* data)
{
	mem::copy(m_pixels, data, sizeof(Colour) * (m_width * m_height));
}

void Image::setPixels(const Colour* data, uint64_t pixel_count)
{
	mem::copy(m_pixels, data, sizeof(Colour) * pixel_count);
}

void Image::setPixels(const Colour* data, uint64_t offset, uint64_t pixel_count)
{
	mem::copy(m_pixels, data + offset, sizeof(Colour) * pixel_count);
}

bool Image::saveToPng(const char* file) const
{
	FileStream fs(file, "wb");
	return saveToPng(fs);
}

bool Image::saveToPng(Stream& stream) const
{
	LLT_ASSERT(m_pixels, "[IMAGE|DEBUG] Pixel data cannot be null.");
	LLT_ASSERT(m_width > 0 && m_height > 0, "[IMAGE|DEBUG] Width and Height must be > 0.");

	stbi_write_force_png_filter = 0;
	stbi_write_png_compression_level = 0;

	if (stbi_write_png_to_func(_stbi_write, &stream, m_width, m_height, 4, m_pixels, m_width * 4) != 0) {
		return true;
	} else {
		LLT_ERROR("[IMAGE|DEBUG] stbi_write_png_to_func(...) failed.");
	}

	return false;
}

bool Image::saveToJpg(const char* file, int quality) const
{
	FileStream fs(file, "wb");
	return saveToJpg(fs, quality);
}

bool Image::saveToJpg(Stream& stream, int quality) const
{
	LLT_ASSERT(m_pixels, "[IMAGE|DEBUG] Pixel data cannot be null.");
	LLT_ASSERT(m_width > 0 && m_height > 0, "[IMAGE|DEBUG] Width and Height must be > 0.");

	if (quality < 1) {
		LLT_LOG("[IMAGE] JPG quality value should be between [1 -> 100].");
		quality = 1;
	} else if (quality > 100) {
		LLT_LOG("[IMAGE] JPG quality value should be between [1 -> 100].");
		quality = 100;
	}

	if (stbi_write_jpg_to_func(_stbi_write, &stream, m_width, m_height, 4, m_pixels, quality) != 0) {
		return true;
	} else {
		LLT_ERROR("[IMAGE|DEBUG] stbi_write_jpg_to_func(...) failed.");
	}

	return false;
}

Colour Image::getPixelAt(uint32_t x, uint32_t y) const
{
	return m_pixels[(y * m_width) + x];
}

Colour* Image::getPixels() { return m_pixels; }
const Colour* Image::getPixels() const { return m_pixels; }
byte* Image::getData() { return (byte*)m_pixels; }
const byte* Image::getData() const { return (const byte*)m_pixels; }
uint32_t Image::getWidth() const { return m_width; }
uint32_t Image::getHeight() const { return m_height; }
uint64_t Image::getSize() const { return m_width * m_height * 4; }
int Image::getNrChannels() const{ return m_nrChannels; }

#pragma once

#include <functional>
#include <string>

#include "math/rect.h"

namespace mgp
{
	struct Colour;
	
	class Stream;
	class PlatformCore;

	class Bitmap
	{
	public:
		enum Format
		{
			FORMAT_RGBA8, // ldr
			FORMAT_RGBAF, // hdr
		};

		Bitmap();
		Bitmap(const std::string &path);
		Bitmap(const char *path);
		Bitmap(int width, int height);

		~Bitmap();

		void load(const std::string &path);
		void load(const char *path);

		void free();

		void paint(const std::function<Colour(uint32_t, uint32_t)> &brush);
		void paint(const RectI &rect, const std::function<Colour(uint32_t, uint32_t)> &brush);

		void setPixels(const Colour *data);
		void setPixels(uint64_t dstFirst, const Colour *data, uint64_t srcFirst, uint64_t count);

		bool saveToPng(PlatformCore *platform, const char *file) const;
		bool saveToPng(Stream &stream) const;
		bool saveToJpg(PlatformCore *platform, const char *file, int quality) const;
		bool saveToJpg(Stream &stream, int quality) const;

		Colour getPixelAt(uint32_t x, uint32_t y) const;

		void *getData();
		const void *getData() const;

		Format getFormat() const;

		uint32_t getWidth() const;
		uint32_t getHeight() const;
		
		uint32_t getPixelCount() const;
		
		uint64_t getMemorySize() const;
		
		int getChannelCount() const;

	private:
		void *m_pixels;
		Format m_format;

		uint32_t m_width;
		uint32_t m_height;
		int m_channels;

		bool m_stbiManaged;
	};
}

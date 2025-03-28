#ifndef IMAGE_H_
#define IMAGE_H_

#include "container/string.h"
#include "container/function.h"

#include "io/file_stream.h"

#include "math/rect.h"

namespace llt
{
	struct Colour;

	class Image
	{
	public:
		enum Format
		{
			FORMAT_RGBA8, // ldr
			FORMAT_RGBAF, // hdr
		};

		using BrushFn = Function<Colour(uint32_t, uint32_t)>;

		Image();
		Image(const String &path);
		Image(const char *path);
		Image(int width, int height);
		~Image();

		void load(const String &path);
		void load(const char *path);

		void free();

		/*
		* Create custom images by defining a "brush" function
		* which is ran on every pixel of the image and outputs
		* a colour for each pixel.
		*/
		void paint(const BrushFn &brush);
		void paint(const RectI &rect, const BrushFn &brush);
		
		void setPixels(const Colour *data);
		void setPixels(uint64_t dstFirst, const Colour *data, uint64_t srcFirst, uint64_t count);

		bool saveToPng(const char *file) const;
		bool saveToPng(Stream &stream) const;
		bool saveToJpg(const char *file, int quality) const;
		bool saveToJpg(Stream &stream, int quality) const;

		Colour getPixelAt(uint32_t x, uint32_t y) const;

		void *getData();
		const void *getData() const;

		Format getFormat() const;
		uint32_t getWidth() const;
		uint32_t getHeight() const;
		uint32_t getPixelCount() const;
		uint64_t getSize() const;
		int getChannels() const;

	private:
		void *m_pixels;
		Format m_format;

		uint32_t m_width;
		uint32_t m_height;
		int m_channels;

		bool m_stbiManaged;
	};
}

#endif // IMAGE_H_

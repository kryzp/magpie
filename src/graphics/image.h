#ifndef IMAGE_H_
#define IMAGE_H_

#include "../container/function.h"
#include "../io/file_stream.h"
#include "../math/rect.h"

namespace llt
{
	struct Colour;

	/**
	* Bitmap image wrapper.
	*/
	class Image
	{
	public:
		using BrushFn = Function<Colour(uint32_t, uint32_t)>;

		Image();
		Image(const char* path);
		Image(int width, int height);
		~Image();

		/*
		* Load the image from a path.
		*/
		void load(const char* path);

		/*
		* Free the image memory.
		*/
		void free();

		/*
		* Create custom images by defining a "brush" function
		* which is ran on every pixel of the image and outputs
		* a colour for each pixel.
		*/
		void paint(const BrushFn& brush);
		void paint(const RectI& rect, const BrushFn& brush);
		
		/*
		* Set the pixels of the image given colour data.
		*/
		void setPixels(const Colour* data);
		void setPixels(const Colour* data, uint64_t pixel_count);
		void setPixels(const Colour* data, uint64_t offset, uint64_t pixelCount);

		/*
		* Writes out the image to a file or into a stream.
		*/
		bool saveToPng(const char* file) const;
		bool saveToPng(Stream& stream) const;
		bool saveToJpg(const char* file, int quality) const;
		bool saveToJpg(Stream& stream, int quality) const;

		/*
		* Get the pixel at a point.
		*/
		Colour getPixelAt(uint32_t x, uint32_t y) const;

		/*
		* Get the pixel data.
		*/
		Colour* getPixels();
		const Colour* getPixels() const;
		byte* getData();
		const byte* getData() const;

		/*
		* Get other meta-data about the image.
		*/
		uint32_t getWidth() const;
		uint32_t getHeight() const;
		uint64_t getSize() const;
		int getNrChannels() const;

	private:
		Colour* m_pixels;

		uint32_t m_width;
		uint32_t m_height;
		int m_nrChannels;

		bool m_stbiManaged;
	};
}

#endif // IMAGE_H_

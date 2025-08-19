
		typedef enum BitmapImageFormat
{
	BitmapImageFormat_RGBA8, // LDR
	BitmapImageFormat_RGBAF  // HDR
}
BitmapImageFormat;

typedef struct BitmapImage
{
	void *pixels;
	BitmapImageFormat format;
	
	i32 width;
	i32 height;
	
	i32 channels;
}
BitmapImage;

typedef struct Font
{
	// TODO(kp)
}
Font;


typedef enum BitmapImageFormat {
	BitmapImageFormat_RGBA8, // LDR
	BitmapImageFormat_RGBAF // HDR
} BitmapImageFormat;

typedef struct BitmapImage {
	void *pixels;
	BitmapImageFormat format;

	i32 width;
	i32 height;

	i32 channels;
} BitmapImage;

/*
typedef struct Font
{
	// TODO
}
Font;
*/

typedef struct AssetTexture {
	String8 path;
	Image image;
} AssetTexture;

typedef struct AssetModel {
	String8 path;
	Model model;
} AssetModel;

typedef struct Assets {
	MemoryArena *arena;

	u32 texture_count;
	AssetTexture textures[128];

	u32 model_count;
	AssetModel models[64];
} Assets;

internal Image *AssetsImageFromHandle(Assets *assets, u32 handle);
internal Model *AssetsModelFromHandle(Assets *assets, u32 handle);

#ifndef ASSETS_H
#define ASSETS_H

#include "core/core_types.h"
#include "core/core_string.h"
#include "core/core_memory_arena.h"

#include "rendering/device.h"
#include "rendering/model.h"

#include "asset_handle.h"

enum bitmap_image_format {
	BITMAP_IMAGE_FORMAT_RGBA8, // LDR
	BITMAP_IMAGE_FORMAT_RGBAF // HDR
};

struct bitmap_image {
	void *pixels;
	enum bitmap_image_format format;
	int width;
	int height;
	int channels;
};

struct bitmap_image bitmap_image_load_from_file(struct string8 path);
void bitmap_image_destroy(struct bitmap_image *bitmap);
u64 bitmap_image_get_memory_size(const struct bitmap_image *bitmap);
struct gfx_texture bitmap_create_gfx_texture(struct bitmap_image *bitmap, struct gfx_device *device);

struct asset_texture {
	struct string8 path;
	struct gfx_texture texture;
};

struct asset_model {
	struct string8 path;
	struct gfx_model model;
};

struct asset_store {
	struct memory_arena *arena;

	u32 texture_count;
	struct asset_texture textures[128];

	u32 model_count;
	struct asset_model models[64];
};

void asset_store_init(struct asset_store *assets, struct memory_arena *arena);
void asset_store_destroy(struct asset_store *assets, struct gfx_device *device);

struct asset_handle asset_store_load_texture(struct asset_store *assets, struct gfx_device *device, struct string8 path);
struct asset_handle asset_store_load_model(struct asset_store *assets, struct gfx_device *device, struct string8 path);
	
struct gfx_texture *asset_store_texture_from_handle(struct asset_store *assets, struct asset_handle handle);
struct gfx_model *asset_store_model_from_handle(struct asset_store *assets, struct asset_handle handle);

#endif // ASSETS_H

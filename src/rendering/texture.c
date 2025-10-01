#include "texture.h"

u32 gfx_texture_face_count(const struct gfx_texture *texture)
{
	return texture->is_cubemap ? 6 : 1;
}

u32 gfx_texture_layer_count(const struct gfx_texture *texture)
{
	if (texture->type == VK_IMAGE_VIEW_TYPE_1D_ARRAY ||
	    texture->type == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
		return texture->depth;

	return gfx_texture_face_count(texture);
}

enum gfx_texture_access_type gfx_texture_get_access_type(const struct gfx_texture *texture,
							 u32 layer, u32 level, u32 aspect)
{
	return texture->access_types[((aspect * gfx_texture_layer_count(texture)) + layer) * texture->mipmap_count + level];
}

void gfx_texture_set_access_type(struct gfx_texture *texture,
				 u32 layer, u32 level, u32 aspect,
				 enum gfx_texture_access_type type)
{
	texture->access_types[((aspect * gfx_texture_layer_count(texture)) + layer) * texture->mipmap_count + level] = type;
}

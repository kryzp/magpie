#ifndef GFX_RESOURCES_H
#define GFX_RESOURCES_H

#include "data/hash_table.h"

#include "sampler.h"
#include "texture.h"
#include "buffer.h"

struct gfx_resource_database {
	struct hash_table samplers;
	struct hash_table textures;
	struct hash_table buffers;
};

void gfx_resource_database_init(struct gfx_resource_database *db);
void gfx_resource_database_destroy(struct gfx_resource_database *db);

void gfx_resource_database_register_sampler(struct gfx_resource_database *db, struct string8 name,
					    const struct gfx_sampler *sampler);

void gfx_resource_database_register_texture(struct gfx_resource_database *db, struct string8 name,
					    const struct gfx_texture *texture);

void gfx_resource_database_register_buffer(struct gfx_resource_database *db, struct string8 name,
					   const struct gfx_buffer *buffer);

struct gfx_sampler *gfx_resource_database_fetch_sampler(struct gfx_resource_database *db, struct string8 name);
struct gfx_texture *gfx_resource_database_fetch_texture(struct gfx_resource_database *db, struct string8 name);
struct gfx_buffer  *gfx_resource_database_fetch_buffer(struct gfx_resource_database *db, struct string8 name);

struct gfx_resource_lookup {
	struct hash_table binding_to_resource;
};

void gfx_resource_lookup_add(struct gfx_resource_lookup *lookup, struct string8 binding, struct string8 resource);
struct string8 gfx_resource_lookup_find(struct gfx_resource_lookup *lookup, struct string8 binding);

#endif // GFX_RESOURCES_H

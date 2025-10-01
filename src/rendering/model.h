#ifndef GFX_MODEL_H
#define GFX_MODEL_H

#include "core/core_types.h"
#include "assets/asset_handle.h"

#include "device.h"

struct gfx_mesh {
	u64 vertex_size;

	u32 vertex_count;
	u32 index_count;
	
	struct gfx_buffer vertex_buffer;
	struct gfx_buffer index_buffer;
};

static inline bool gfx_meshes_equal(struct gfx_mesh *a, struct gfx_mesh *b)
{
	return (a->vertex_size          == b->vertex_size &&
		a->vertex_count         == b->vertex_count &&
		a->index_count          == b->index_count &&
		a->vertex_buffer.handle == b->vertex_buffer.handle &&
		a->index_buffer.handle  == b->index_buffer.handle);
}

void gfx_mesh_init(struct gfx_mesh *mesh, struct gfx_device *device,
		   u64 vertex_size,
		   u32 vertex_count, void *vertices,
		   u32 index_count, u16 *indices);

void gfx_mesh_destroy(struct gfx_mesh *mesh, struct gfx_device *device);

void gfx_mesh_bind_indices(struct gfx_mesh *mesh, struct gfx_command_buffer *cmd);
void gfx_mesh_draw_indexed(struct gfx_mesh *mesh, struct gfx_command_buffer *cmd);
void gfx_mesh_draw_indexed_id(struct gfx_mesh *mesh, struct gfx_command_buffer *cmd, u32 instance_id);

/*
enum gfx_transparency_mode
{
	GFX_TRANSPARENCY_MODE_opaque,
	GFX_TRANSPARENCY_MODE_transparent,
	GFX_TRANSPARENCY_MODE_max_enum
};

struct gfx_material_shader
{
	struct gfx_shader_program passes[GFX_MESH_BATCH_TYPE_max_enum];
	enum gfx_transparency_mode transparency;
};
*/

struct gfx_material {
	//struct gfx_material_shader *shader;

	struct asset_handle diffuse_texture_handle;
	//v3 diffuse_factor;

	struct asset_handle normal_texture_handle;
	//float normal_factor;

	struct asset_handle emissive_texture_handle;
	//v3 emissive_factor;

	struct asset_handle metallic_roughness_texture_handle;
	//float metallic_factor;
	//float roughness_factor;

	struct asset_handle ambient_texture_handle;
	//float ambient_factor;
};

static inline bool gfx_materials_equal(struct gfx_material *a, struct gfx_material *b)
{
	return (asset_handles_equal(a->diffuse_texture_handle, b->diffuse_texture_handle) &&
		asset_handles_equal(a->normal_texture_handle, b->normal_texture_handle) &&
		asset_handles_equal(a->emissive_texture_handle, b->emissive_texture_handle) &&
		asset_handles_equal(a->metallic_roughness_texture_handle, b->metallic_roughness_texture_handle) &&
		asset_handles_equal(a->ambient_texture_handle, b->ambient_texture_handle));
}

struct gfx_model;

struct gfx_sub_model {
	struct gfx_sub_model *next;
	struct gfx_model *parent;
	struct gfx_mesh mesh;
	struct gfx_material material;
};

struct gfx_model {
	struct memory_arena *arena;
	struct gfx_sub_model *sub_models;
};

void gfx_model_init(struct gfx_model *model);
void gfx_model_destroy(struct gfx_model *model, struct gfx_device *device);

struct gfx_sub_model *gfx_model_add_sub_model(struct gfx_model *model);

#endif // GFX_MODEL_H

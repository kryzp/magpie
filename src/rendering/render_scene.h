#ifndef GFX_RENDER_SCENE_H
#define GFX_RENDER_SCENE_H

#include "core/core_types.h"
#include "core/core_math.h"

#include "texture.h"
#include "buffer.h"
#include "model.h"
#include "camera.h"
#include "gpu_types.h"

struct asset_store;

enum gfx_light_type {
	GFX_LIGHT_TYPE_point,
	GFX_LIGHT_TYPE_max_enum
};

struct gfx_light {
	enum gfx_light_type type;
	v3 direction;
	v3 colour;
	float intensity;
	float falloff;
};

struct gfx_environment_probe {
	struct gfx_texture irradiance;
	struct gfx_texture prefilter;
};

struct gfx_environment_probe gfx_environment_probe_alloc(struct gfx_device *device);
void gfx_environment_probe_destroy(struct gfx_environment_probe *probe, struct gfx_device *device);

struct gfx_multi_batch {
	u32 first;
	u32 count;
};

struct gfx_indirect_batch {
	u32 mesh_id;
	u32 material_id;
	u32 first;
	u32 count;
};

struct gfx_direct_batch {
	u32 object_id;
	//u64 sort_key;
};

enum gfx_mesh_pass_type {
	GFX_MESH_PASS_forward,
	GFX_MESH_PASS_max_enum
};

struct gfx_mesh_pass {
	// Instanced Draws.
	u32 multi_batch_count;
	struct gfx_multi_batch multi_batches[32];

	// Indirect Draws.
	u32 batch_count;
	struct gfx_indirect_batch batches[32];

	// Direct Draws.
	// TODO: This still needs to be actually sorted!
	u32 direct_batch_count;
	struct gfx_direct_batch direct_batches[32];

	struct gfx_buffer compacted_instance_buffer; // <u32>
	struct gfx_buffer instance_buffer;           // <gfx_gpu_instance>
	struct gfx_buffer draw_indirect_buffer;      // <gfx_gpu_indirect>
	struct gfx_buffer clear_indirect_buffer;     // <gfx_gpu_indirect>

	bool instance_buffer_refresh;
	bool indirect_buffer_refresh;
};

struct gfx_render_scene;

void gfx_mesh_pass_init(struct gfx_mesh_pass *pass, struct gfx_device *device);
void gfx_mesh_pass_destroy(struct gfx_mesh_pass *pass, struct gfx_device *device);

void gfx_mesh_pass_populate(struct gfx_mesh_pass *pass, struct gfx_render_scene *scene);

void gfx_mesh_pass_fill_instances_array(struct gfx_mesh_pass *pass, struct gfx_render_scene *scene, struct gfx_gpu_instance *instances);
void gfx_mesh_pass_fill_indirect_array(struct gfx_mesh_pass *pass, struct gfx_render_scene *scene, struct gfx_gpu_indirect *indirects);

struct gfx_render_mesh {
	struct gfx_mesh *original;
	bool is_merged;
	u32 first_vertex;
	u32 first_index;
	u32 vertex_count;
	u32 index_count;
};

#define GFX_RS_HANDLE_INVALID_INDEX ((u32)(-1))

static inline bool gfx_rs_handle_valid(u32 h)
{
	return h != GFX_RS_HANDLE_INVALID_INDEX && h >= 0;
};

struct gfx_render_scene_object {
	struct gfx_render_scene_object *next;
	u32 id;
	m4 transform;
	//struct gfx_bounds3d bounds;
	u32 flags;
	//u64 custom_sort_key;
	u32 mesh_handle;
	u32 material_handle;
	u32 light_handle;
};

void gfx_render_scene_object_init(struct gfx_render_scene_object *object);

struct gfx_render_view {
	struct gfx_camera *camera;
	struct gfx_render_scene *scene;
};

#define GFX_RENDER_SCENE_MAX_MATERIALS 128
#define GFX_RENDER_SCENE_MAX_OBJECTS 256

// This is seperate from the entity scene.
// -- An entity scene is used to create a render scene.
//    And a render scene is used to render the world.
struct gfx_render_scene {

	struct memory_arena *arena;
	
	// Linked freelist of objects.
	u32 object_count;
	struct gfx_render_scene_object *first_free_object;
	struct gfx_render_scene_object *objects;

	// Object handles pending removal.
	u32 pending_removal_count;
	u32 pending_removal[GFX_RENDER_SCENE_MAX_OBJECTS];

	// Object handles available for reuse.
	u32 reusable_handle_count;
	u32 reusable_handles[GFX_RENDER_SCENE_MAX_OBJECTS];
	
	struct gfx_mesh_pass mesh_passes[GFX_MESH_PASS_max_enum];

	u32 mesh_count;
	struct gfx_render_mesh meshes[GFX_RENDER_SCENE_MAX_OBJECTS];

	u32 material_count;
	struct gfx_material materials[GFX_RENDER_SCENE_MAX_MATERIALS];

	u32 light_count;
	struct gfx_light lights[GFX_RENDER_SCENE_MAX_OBJECTS];

	struct gfx_buffer object_buffer;
	struct gfx_buffer material_buffer;
	struct gfx_buffer light_buffer;

	struct gfx_buffer merged_vertex_buffer;
	struct gfx_buffer merged_index_buffer;
};

void gfx_render_scene_init(struct gfx_render_scene *scene, struct gfx_device *device, struct memory_arena *arena);
void gfx_render_scene_destroy(struct gfx_render_scene *scene, struct gfx_device *device);

void gfx_render_scene_update(struct gfx_render_scene *scene, struct gfx_device *device);

void gfx_render_scene_remove_object(struct gfx_render_scene *scene, u32 handle);
void gfx_render_scene_resolve_removing(struct gfx_render_scene *scene);

u32 gfx_render_scene_create_object(struct gfx_render_scene *scene, m4 transform);

u32 gfx_render_scene_upload_mesh(struct gfx_render_scene *scene, struct gfx_mesh *mesh);
u32 gfx_render_scene_upload_material(struct gfx_render_scene *scene, struct gfx_device *device, struct asset_store *assets, struct gfx_material *material);
u32 gfx_render_scene_upload_light(struct gfx_render_scene *scene, struct gfx_light *light);

void gfx_render_scene_merge_meshes(struct gfx_render_scene *scene, struct gfx_device *device);

struct gfx_render_scene_object *gfx_render_scene_object_from_handle(struct gfx_render_scene *scene, u32 handle);

#endif // GFX_RENDER_SCENE_H

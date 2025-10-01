#include "render_scene.h"

#include "assets/assets.h"

struct gfx_environment_probe gfx_environment_probe_alloc(struct gfx_device *device)
{
	struct gfx_environment_probe probe = {0};
	probe.irradiance = gfx_device_texture_alloc_cubemap(device, 32, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
	probe.prefilter  = gfx_device_texture_alloc_cubemap(device, 128, VK_FORMAT_R32G32B32A32_SFLOAT, 4);

	return probe;
}

void gfx_environment_probe_destroy(struct gfx_environment_probe *probe, struct gfx_device *device)
{
	gfx_device_texture_destroy(device, &probe->irradiance);
	gfx_device_texture_destroy(device, &probe->prefilter);
}

void gfx_mesh_pass_init(struct gfx_mesh_pass *pass, struct gfx_device *device)
{
	pass->compacted_instance_buffer = gfx_device_buffer_alloc(device,
								  VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
								  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
								  sizeof(u32) * GFX_RENDER_SCENE_MAX_OBJECTS);

	pass->instance_buffer = gfx_device_buffer_alloc(device,
							VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
							VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
							VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							sizeof(struct gfx_gpu_instance) * GFX_RENDER_SCENE_MAX_OBJECTS);
	
	pass->draw_indirect_buffer = gfx_device_buffer_alloc(device,
							     VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
							     VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
							     VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
							     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							     sizeof(struct gfx_gpu_indirect) * GFX_RENDER_SCENE_MAX_OBJECTS);
	
	pass->clear_indirect_buffer = gfx_device_buffer_alloc(device,
							      VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
							      VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
							      VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
							      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							      sizeof(struct gfx_gpu_indirect) * GFX_RENDER_SCENE_MAX_OBJECTS);
}

void gfx_mesh_pass_destroy(struct gfx_mesh_pass *pass, struct gfx_device *device)
{
	gfx_device_buffer_destroy(device, &pass->compacted_instance_buffer);
	gfx_device_buffer_destroy(device, &pass->instance_buffer);
	gfx_device_buffer_destroy(device, &pass->draw_indirect_buffer);
	gfx_device_buffer_destroy(device, &pass->clear_indirect_buffer);
}

/*
 * RENDER OBJECTS -> DIRECT BATCHES -> (INDIRECT) BATCHES -> MULTI BATCHES.
 */
void gfx_mesh_pass_populate(struct gfx_mesh_pass *pass, struct gfx_render_scene *scene)
{
	// RESET.
	pass->multi_batch_count = 0;
	pass->batch_count = 0;
	pass->direct_batch_count = 0;
	
	// DIRECT BATCHES.
	for (struct gfx_render_scene_object *s = scene->objects; s; s = s->next) {
		if (!gfx_rs_handle_valid(s->mesh_handle))
			continue;

		struct gfx_direct_batch *direct_batch = pass->direct_batches + pass->direct_batch_count;
		direct_batch->object_id = s->id;

		pass->direct_batch_count++;
	}

	// BATCHES.
	struct gfx_render_scene_object *object = gfx_render_scene_object_from_handle(scene, pass->direct_batches->object_id);

	struct gfx_indirect_batch *batch = pass->batches;
	batch->mesh_id = object->mesh_handle;
	batch->material_id = object->material_handle;
	batch->first = 0;
	batch->count = 1;

	pass->batch_count = 1;

	for (int i = 1; i < pass->direct_batch_count; i++) {
		struct gfx_direct_batch *curr = pass->direct_batches + i;
		object = gfx_render_scene_object_from_handle(scene, curr->object_id);

		bool are_same_mesh     = object->mesh_handle     == batch->mesh_id;
		bool are_same_material = object->material_handle == batch->material_id;

		if (are_same_mesh && are_same_material) {
			batch->count++;
		} else {
			batch = pass->batches + pass->batch_count;
			batch->mesh_id = object->mesh_handle;
			batch->material_id = object->material_handle;
			batch->first = i;
			batch->count = 1;

			pass->batch_count++;
		}
	}

	// MULTI BATCHES.
	struct gfx_multi_batch *multi_batch = pass->multi_batches;
	multi_batch->count = 1;
	multi_batch->first = 0;

	pass->multi_batch_count = 1;
	
	for (int i = 1; i < pass->batch_count; i++) {
		struct gfx_indirect_batch *curr_batch = pass->batches + i;
		struct gfx_indirect_batch *join_batch = pass->batches + multi_batch->first;
		
		bool compatible_mesh = scene->meshes[join_batch->mesh_id].is_merged;
		bool same_material = curr_batch->material_id == join_batch->material_id;
		
		// As long as the materials are the same and the mesh
		// has been merged then we can combine the rendering
		// calls together.
		if (compatible_mesh && same_material) {
			multi_batch->count++;
		} else {
			multi_batch = pass->multi_batches + pass->multi_batch_count;
			multi_batch->count = 1;
			multi_batch->first = i;

			pass->multi_batch_count++;
		}
	}
}

void gfx_mesh_pass_fill_instances_array(struct gfx_mesh_pass *pass, struct gfx_render_scene *scene, struct gfx_gpu_instance *instances)
{
	int index = 0;
	
	for (int i = 0; i < pass->batch_count; i++) {
		struct gfx_indirect_batch *b = pass->batches + i;
		
		for (int k = 0; k < b->count; k++) {
			struct gfx_direct_batch *direct_batch = pass->direct_batches + b->first + k;

			instances[index].object_id = direct_batch->object_id;
			instances[index].batch_id = i;
			
			index++;
		}
	}
}

void gfx_mesh_pass_fill_indirect_array(struct gfx_mesh_pass *pass, struct gfx_render_scene *scene, struct gfx_gpu_indirect *indirects)
{
	for (int i = 0; i < pass->batch_count; i++) {
		struct gfx_indirect_batch *b = pass->batches + i;
		struct gfx_render_mesh *mesh = scene->meshes + b->mesh_id;
		
		indirects[i].command.firstInstance = b->first;
		indirects[i].command.instanceCount = 0; // This gets filled-in in the compute shader.
		indirects[i].command.vertexOffset = mesh->first_vertex;
		indirects[i].command.firstIndex = mesh->first_index;
		indirects[i].command.indexCount = mesh->index_count;
	}
}

void gfx_render_scene_object_init(struct gfx_render_scene_object *object)
{
	object->id              = GFX_RS_HANDLE_INVALID_INDEX;
	object->mesh_handle     = GFX_RS_HANDLE_INVALID_INDEX;
	object->material_handle = GFX_RS_HANDLE_INVALID_INDEX;
	object->light_handle    = GFX_RS_HANDLE_INVALID_INDEX;
	object->transform       = m4(1.f);
	object->flags           = 0;
}

void gfx_render_scene_init(struct gfx_render_scene *scene, struct gfx_device *device, struct memory_arena *arena)
{
	scene->arena = arena;

	scene->object_buffer = gfx_device_buffer_alloc(device,
						       VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
						       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
						       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						       sizeof(struct gfx_gpu_object_data) * GFX_RENDER_SCENE_MAX_OBJECTS);

	scene->material_buffer = gfx_device_buffer_alloc(device,
							 VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
							 VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
							 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							 sizeof(struct gfx_gpu_material) * GFX_RENDER_SCENE_MAX_MATERIALS);

	scene->light_buffer = gfx_device_buffer_alloc(device,
						      VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
						      VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
						      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						      sizeof(struct gfx_gpu_light) * GFX_RENDER_SCENE_MAX_OBJECTS);

	for (int i = 0; i < GFX_MESH_PASS_max_enum; i++)
		gfx_mesh_pass_init(scene->mesh_passes + i, device);
}

void gfx_render_scene_destroy(struct gfx_render_scene *scene, struct gfx_device *device)
{
	gfx_device_buffer_destroy(device, &scene->merged_vertex_buffer);
	gfx_device_buffer_destroy(device, &scene->merged_index_buffer);

	gfx_device_buffer_destroy(device, &scene->object_buffer);
	gfx_device_buffer_destroy(device, &scene->material_buffer);
	gfx_device_buffer_destroy(device, &scene->light_buffer);
	
	for (int i = 0; i < GFX_MESH_PASS_max_enum; i++)
		gfx_mesh_pass_destroy(scene->mesh_passes + i, device);
}

void gfx_render_scene_update(struct gfx_render_scene *scene, struct gfx_device *device)
{
}

void gfx_render_scene_remove_object(struct gfx_render_scene *scene, u32 handle)
{
	scene->pending_removal[scene->pending_removal_count++] = handle;
}

void gfx_render_scene_resolve_removing(struct gfx_render_scene *scene)
{
	for (int i = 0; i < scene->pending_removal_count; i++) {
		u32 to_remove_handle = scene->pending_removal[i];
		struct gfx_render_scene_object *to_remove = gfx_render_scene_object_from_handle(scene, to_remove_handle);
		to_remove->next = scene->first_free_object;
		scene->first_free_object = to_remove;
		scene->reusable_handles[scene->reusable_handle_count++] = to_remove_handle;
		scene->object_count--;
	}

	scene->pending_removal_count = 0;
}

u32 gfx_render_scene_create_object(struct gfx_render_scene *scene, m4 transform)
{
	assert(scene->object_count < GFX_RENDER_SCENE_MAX_OBJECTS);
	
	u32 handle = scene->object_count;

	if (scene->reusable_handle_count > 0)
		handle = scene->reusable_handles[--scene->reusable_handle_count];

	struct gfx_render_scene_object *object = scene->first_free_object;

	if (object) {
		scene->first_free_object = scene->first_free_object->next;
		memory_zero_struct(object);
	} else {
		object = memory_arena_push(scene->arena, sizeof(struct gfx_render_scene_object));
	}
	
	gfx_render_scene_object_init(object);
	
	object->id = handle;
	object->transform = transform;

	object->next = scene->objects;
	scene->objects = object;
	
	scene->object_count++;

	return handle;
}

u32 gfx_render_scene_upload_mesh(struct gfx_render_scene *scene, struct gfx_mesh *mesh)
{
	for (int i = 0; i < scene->mesh_count; i++) {
		if (gfx_meshes_equal(scene->meshes[i].original, mesh))
			return i;
	}

	struct gfx_render_mesh render_mesh = {0};
	render_mesh.original = mesh;
	render_mesh.is_merged = false;
	render_mesh.first_vertex = 0;
	render_mesh.first_index = 0;
	render_mesh.vertex_count = mesh->vertex_count;
	render_mesh.index_count = mesh->index_count;

	scene->meshes[scene->mesh_count] = render_mesh;

	return scene->mesh_count++;
}

u32 gfx_render_scene_upload_material(struct gfx_render_scene *scene, struct gfx_device *device, struct asset_store *assets, struct gfx_material *material)
{
	for (int i = 0; i < scene->material_count; i++) {
		if (gfx_materials_equal(&scene->materials[i], material))
			return i;
	}

	scene->materials[scene->material_count] = *material;
	
	struct gfx_gpu_material gpu_material = {0};
	gpu_material.diffuse_texture            = gfx_device_texture_view_fetch_std(device, asset_store_texture_from_handle(assets, material->diffuse_texture_handle))->bindless.sampled;
	gpu_material.normal_texture             = gfx_device_texture_view_fetch_std(device, asset_store_texture_from_handle(assets, material->normal_texture_handle))->bindless.sampled;
	gpu_material.emissive_texture           = gfx_device_texture_view_fetch_std(device, asset_store_texture_from_handle(assets, material->emissive_texture_handle))->bindless.sampled;
	gpu_material.metallic_roughness_texture = gfx_device_texture_view_fetch_std(device, asset_store_texture_from_handle(assets, material->metallic_roughness_texture_handle))->bindless.sampled;
	gpu_material.ambient_texture            = gfx_device_texture_view_fetch_std(device, asset_store_texture_from_handle(assets, material->ambient_texture_handle))->bindless.sampled;

	gfx_buffer_write(&scene->material_buffer, &gpu_material,
			 sizeof(struct gfx_gpu_material),
			 sizeof(struct gfx_gpu_material) * scene->material_count);

	return scene->material_count++;
}

u32 gfx_render_scene_upload_light(struct gfx_render_scene *scene, struct gfx_light *light)
{
	scene->lights[scene->light_count] = *light;
	
	u32 handle = scene->light_count;

	scene->light_count++;
	return handle;
}

void gfx_render_scene_merge_meshes(struct gfx_render_scene *scene, struct gfx_device *device)
{
	if (scene->mesh_count <= 0)
		return;

	// All meshes in the list *should* have the same vertex type.
	// If they don't we have a bit of a problem :/.
	u64 vertex_size = scene->meshes->original->vertex_size;

	u32 total_vertices = 0;
	u32 total_indices = 0;

	for (int i = 0; i < scene->mesh_count; i++) {
		struct gfx_render_mesh *mesh = &scene->meshes[i];

		mesh->first_vertex = total_vertices;
		mesh->first_index = total_indices;

		total_vertices += mesh->vertex_count;
		total_indices += mesh->index_count;

		mesh->is_merged = true;
	}

	u64 vb_size = total_vertices * vertex_size;
	u64 ib_size = total_indices  * sizeof(u16);
	
	if (scene->merged_vertex_buffer.size < vb_size ||
	    scene->merged_index_buffer.size < ib_size) {
		
		gfx_device_buffer_destroy(device, &scene->merged_vertex_buffer);
		gfx_device_buffer_destroy(device, &scene->merged_index_buffer);
		
		scene->merged_vertex_buffer = gfx_device_buffer_alloc(device,
								      VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT |
								      VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
								      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
								      vb_size);
	
		scene->merged_index_buffer = gfx_device_buffer_alloc(device,
								     VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
								     VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
								     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
								     ib_size);
	}
	
	struct gfx_command_buffer cmd = gfx_device_begin_instant_submit(device);

	for (int i = 0; i < scene->mesh_count; i++) {
		struct gfx_render_mesh *mesh = scene->meshes + i;

		VkBufferCopy vertex_copy = {0};
		vertex_copy.srcOffset = 0;
		vertex_copy.dstOffset = mesh->first_vertex * vertex_size;
		vertex_copy.size      = mesh->vertex_count * vertex_size;
		
		VkBufferCopy index_copy = {0};
		index_copy.srcOffset = 0;
		index_copy.dstOffset = mesh->first_index * sizeof(u16);
		index_copy.size      = mesh->index_count * sizeof(u16);

		gfx_cmd_copy_buffer_to_buffer(&cmd,
					      &mesh->original->vertex_buffer,
					      &scene->merged_vertex_buffer,
					      1, &vertex_copy);

		gfx_cmd_copy_buffer_to_buffer(&cmd,
					      &mesh->original->index_buffer,
					      &scene->merged_index_buffer,
					      1, &index_copy);
	}
	
	gfx_device_end_instant_submit(device, &cmd);
}

struct gfx_render_scene_object *gfx_render_scene_object_from_handle(struct gfx_render_scene *scene, u32 handle)
{
	assert(handle >= 0 && handle < scene->object_count);
	struct gfx_render_scene_object *o = scene->objects;
	for (u32 i = 0; i < handle; i++, o = o->next);
	return o;
}

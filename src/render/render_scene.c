
internal void
R_SceneInit(R_Scene *scene, Arena *arena, G_Device *device, A_Registry *assets, LOG_Channel log_channel)
{
	MemZeroStruct(scene);

	scene->arena = arena;
	scene->device = device;
	scene->assets = assets;
	scene->log_channel = log_channel;

	for (u32 i = 0; i < ArraySize(scene->object_slots);   i++)  scene->object_slots[i].generation = 1;
	for (u32 i = 0; i < ArraySize(scene->light_slots);    i++)  scene->light_slots[i].generation = 1;
	for (u32 i = 0; i < ArraySize(scene->material_slots); i++)  scene->material_slots[i].generation = 1;
	for (u32 i = 0; i < ArraySize(scene->mesh_slots);     i++)  scene->mesh_slots[i].generation = 1;

	for (i32 i = ArraySize(scene->object_slots) - 1;   i > 0; i--)  scene->object_free_list[scene->object_free_count++] = i - 1;
	for (i32 i = ArraySize(scene->light_slots) - 1;    i > 0; i--)  scene->light_free_list[scene->light_free_count++] = i - 1;
	for (i32 i = ArraySize(scene->material_slots) - 1; i > 0; i--)  scene->material_free_list[scene->material_free_count++] = i - 1;
	for (i32 i = ArraySize(scene->mesh_slots) - 1;     i > 0; i--)  scene->mesh_free_list[scene->mesh_free_count++] = i - 1;

	G_BufferAllocInfo material_buffer_alloc_info = {0};
	material_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	material_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	material_buffer_alloc_info.size = sizeof(R_GPU_Material) * ArraySize(scene->material_slots);
		
	G_BufferAllocInfo mesh_buffer_alloc_info = {0};
	mesh_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	mesh_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	mesh_buffer_alloc_info.size = sizeof(R_GPU_RenderMesh) * ArraySize(scene->mesh_slots);

	scene->material_buffer = G_DeviceBufferAlloc(device, &material_buffer_alloc_info);
	scene->mesh_buffer     = G_DeviceBufferAlloc(device, &mesh_buffer_alloc_info);

	scene->material_buffer_dirty = true;
	scene->mesh_buffer_dirty     = true;

	DebugLogI(scene->log_channel, "Initialized.");
}

internal void
R_SceneDestroy(R_Scene *scene)
{
	G_DeviceBufferDestroy(scene->device, scene->material_buffer);
	G_DeviceBufferDestroy(scene->device, scene->mesh_buffer);

	for (u32 i = 0; i < scene->geometry_page_count; i++)
	{
		G_DeviceBufferDestroy(scene->device, scene->geometry_pages[i].vertex_buffer);
		G_DeviceBufferDestroy(scene->device, scene->geometry_pages[i].index_buffer);
	}
	
	DebugLogI(scene->log_channel, "Destroyed.");
}

internal void
R_SceneDrawIndirect(const R_Scene *scene,
					G_CmdBuffer *cmd,
					G_BufferKey indirect_buffer,
					G_BufferKey count_buffer)
{
	for (u64 i = 0; i < scene->geometry_page_count; i++)
	{
		const R_GeometryPage *page = &scene->geometry_pages[i];

		u64 max_draws_per_page = R_SCENE_MAX_OBJECTS;

		u64 indirect_offset = i * sizeof(R_GPU_IndirectDraw) * max_draws_per_page;
		u64 count_offset    = i * sizeof(u32);
		
		G_CmdBindIndexBuffer(cmd,
							   page->index_buffer,
							   0, VK_WHOLE_SIZE,
							   VK_INDEX_TYPE_UINT32);
		
		G_CmdDrawIndexedIndirectCount(cmd,
										indirect_buffer, indirect_offset,
										count_buffer, count_offset,
										max_draws_per_page,
										sizeof(R_GPU_IndirectDraw));
	}
}

internal R_SceneFrameData
R_SceneUploadFrameData(R_Scene *scene, G_RingBuffer *ring)
{
	R_SceneFrameData resources = {0};

	resources.page_count = scene->geometry_page_count;
	
	if (scene->mesh_buffer_dirty)
	{
		R_SceneFlushMeshBuffer(scene);
		scene->mesh_buffer_dirty = false;
	}

	if (scene->material_buffer_dirty)
	{
		R_SceneFlushMaterialBuffer(scene);
		scene->material_buffer_dirty = false;
	}
 
	R_SceneUploadPageTable(scene, ring, &resources);
 
	if (scene->object_count > 0)
	{
		R_SceneUploadSkinning(scene, ring, &resources);
		R_SceneUploadObjects(scene, ring, &resources);
	}
 
	if (scene->light_count > 0)
	{
		R_SceneUploadLights(scene, ring, &resources);
	}
 
	return resources;
}

internal void
R_SceneUploadPageTable(R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out)
{
	u32 count = scene->geometry_page_count > 0 ? scene->geometry_page_count : 1;

	out->page_table_buffer = G_RingBufferPushArray(ring, R_GPU_PagePointers, count);

	R_GPU_PagePointers *mapped = out->page_table_buffer.cpu;

	for (u32 i = 0; i < scene->geometry_page_count; i++)
		mapped[i].vertex_buffer = G_DeviceBufferAddress(scene->device, scene->geometry_pages[i].vertex_buffer);
}

internal void
R_SceneUploadSkinning(R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out)
{
	u32 total_joints = 0;

	for (u32 i = 0; i < ArraySize(scene->object_slots); i++)
	{
		R_ObjectSlot *slot = &scene->object_slots[i];

		slot->skinning_palette_gpu_addr = 0;
		
		if (!slot->active || slot->skinning_palette == NULL)
			continue;

		total_joints += slot->skinning_joint_count;
	}

	if (total_joints <= 0)
		return;
	
	out->skinning_palette_buffer = G_RingBufferPushArray(ring, m4, total_joints);
 
	m4 *mapped = out->skinning_palette_buffer.cpu;

	u32 offset = 0;

	for (u32 i = 0; i < ArraySize(scene->object_slots); i++)
	{
		R_ObjectSlot *slot = &scene->object_slots[i];

		if (!slot->active || slot->skinning_palette == NULL)
			continue;

		u32 joints = slot->skinning_joint_count;

		MemCopy(mapped + offset, slot->skinning_palette, joints * sizeof(m4));

		slot->skinning_palette_gpu_addr = out->skinning_palette_buffer.gpu + offset * sizeof(m4);

		offset += joints;
	}
}

internal void
R_SceneUploadObjects(R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out)
{
	out->object_buffer = G_RingBufferPushArray(ring, R_GPU_ObjectData, scene->object_count);
 
	R_GPU_ObjectData *mapped = out->object_buffer.cpu;

	u32 write_index = 0;
 	
	for (u32 i = 0; i < ArraySize(scene->object_slots) && write_index < scene->object_count; i++)
	{
		R_ObjectSlot *slot = &scene->object_slots[i];
		
		if (!slot->active)
			continue;

		u32 mesh_index = slot->mesh.index;
		u32 material_index = slot->material.index;
		u32 page_index = scene->mesh_slots[mesh_index].page_index;
 
		mapped[write_index].model_matrix            = slot->transform;
		mapped[write_index].normal_matrix           = M4RemoveTranslation(M4Inverse(M4Transpose(slot->transform)));

		mapped[write_index].sphere_bounds           = slot->sphere_bounds;

		mapped[write_index].material_index          = material_index;
		mapped[write_index].mesh_index	            = mesh_index;
		mapped[write_index].page_index	            = page_index;

		mapped[write_index].skinning_palette_buffer = slot->skinning_palette_gpu_addr;
		mapped[write_index].skinning_joint_count    = slot->skinning_joint_count;
 
		write_index++;
	}
}

internal void
R_SceneUploadLights(R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out)
{
	scene->shadow_caster_count = 0;

	out->light_buffer = G_RingBufferPushArray(ring, R_GPU_Light, scene->light_count);

	R_GPU_Light *mapped = out->light_buffer.cpu;

	u32 write_index = 0;
	i32 shadow_slot_index = 0;

	for (u32 i = 0; i < ArraySize(scene->light_slots) && write_index < scene->light_count; i++)
	{
		R_LightSlot *slot = &scene->light_slots[i];

		if (!slot->active)
			continue;

		const R_Light *light = &slot->light;

		const f32 heuristic_radius = R_LightHeuristicRadius(light, 0.05f);

		mapped[write_index].transform   = M4Transform(light->position, V4QuatIdentity(), v3x(heuristic_radius), v3x(0.f));
		mapped[write_index].position    = light->position;
		mapped[write_index].colour      = light->colour;
		mapped[write_index].intensity   = light->intensity;
		mapped[write_index].attenuation = v3(light->falloff, 0.f, 0.f);
		mapped[write_index].radius      = heuristic_radius;

		if (light->casts_shadows && scene->shadow_caster_count < R_SCENE_MAX_SHADOW_CASTERS)
		{
			mapped[write_index].shadow_slot_index = shadow_slot_index;

			R_ShadowCaster *caster = &scene->shadow_casters[scene->shadow_caster_count];
			caster->position = light->position;
			caster->near     = light->shadow_near;
			caster->far      = light->shadow_far;
			caster->radius   = heuristic_radius;

			scene->shadow_caster_count++;

			shadow_slot_index++;
		}
		else
		{
			mapped[write_index].shadow_slot_index = -1;
		}

		write_index++;
	}
}

internal R_SceneHandle
R_SceneObjectCreate(R_Scene *scene, const R_ObjectDesc *desc)
{
	DebugLogAssert(scene->log_channel,
				   scene->object_free_count > 0,
				   "Ran out of free object slots.");

	scene->object_free_count--;
	u32 slot_index = scene->object_free_list[scene->object_free_count];
	R_ObjectSlot *slot = &scene->object_slots[slot_index];

	slot->transform = desc->transform;
	slot->sphere_bounds = desc->sphere_bounds;
	slot->mesh = desc->mesh;
	slot->material = desc->material;

	slot->skinning_palette = NULL;
	slot->skinning_joint_count = 0;

	slot->active = true;

	scene->object_count++;

	R_SceneHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;

	return handle;
}

internal void
R_SceneObjectDestroy(R_Scene *scene, R_SceneHandle handle)
{
	R_ObjectSlot *slot = R_SceneObjectGetSlot(scene, handle);

	if (!slot)
		return;

	slot->active = false;
	slot->generation++;

	scene->object_free_list[scene->object_free_count++] = handle.index;
	scene->object_count--;
}

internal u32
R_SceneObjectCount(const R_Scene *scene)
{
	return scene->object_count;
}

internal R_ObjectSlot *
R_SceneObjectGetSlot(R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->object_slots))
		return NULL;

	R_ObjectSlot *slot = &scene->object_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return NULL;

	return slot;
}

internal void
R_SceneObjectSetTransform(R_Scene *scene, R_SceneHandle handle, m4 transform)
{
	R_ObjectSlot *slot = R_SceneObjectGetSlot(scene, handle);

	if (!slot)
		return;
	
	slot->transform = transform;
}

internal void
R_SceneObjectSetSphereBounds(R_Scene *scene, R_SceneHandle handle, v4 sphere_bounds)
{
	R_ObjectSlot *slot = R_SceneObjectGetSlot(scene, handle);

	if (!slot)
		return;
	
	slot->sphere_bounds = sphere_bounds;
}

internal void
R_SceneObjectSetMaterial(R_Scene *scene, R_SceneHandle handle, R_SceneHandle material)
{
	R_ObjectSlot *slot = R_SceneObjectGetSlot(scene, handle);

	if (!slot)
		return;
	
	slot->material = material;
}

internal void
R_SceneObjectSetMesh(R_Scene *scene, R_SceneHandle handle, R_SceneHandle mesh)
{
	R_ObjectSlot *slot = R_SceneObjectGetSlot(scene, handle);

	if (!slot)
		return;
	
	slot->mesh = mesh;
}

internal void
R_SceneObjectSetSkinning(R_Scene *scene, R_SceneHandle handle, const AN_Palette *palette)
{
	R_ObjectSlot *slot = R_SceneObjectGetSlot(scene, handle);

	if (!slot)
		return;
	
	slot->skinning_palette = palette->palette;
	slot->skinning_joint_count = palette->joint_count;
}

internal b32
R_SceneObjectHandleIsValid(const R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->object_slots))
		return false;

	const R_ObjectSlot *slot = &scene->object_slots[handle.index];
	return slot->active && slot->generation == handle.generation;
}

internal R_SceneHandle
R_SceneLightCreate(R_Scene *scene, const R_Light *light)
{
	DebugLogAssert(scene->log_channel,
				   scene->light_free_count > 0,
				   "Ran out of free light slots.");

	scene->light_free_count--;
	u32 slot_index = scene->light_free_list[scene->light_free_count];
	R_LightSlot *slot = &scene->light_slots[slot_index];

	slot->light = *light;
	
	slot->active = true;

	scene->light_count++;

	R_SceneHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;

	return handle;
}

internal void
R_SceneLightDestroy(R_Scene *scene, R_SceneHandle handle)
{
	R_LightSlot *slot = R_SceneLightGetSlot(scene, handle);

	if (!slot)
		return;

	slot->active = false;
	slot->generation++;

	scene->light_free_list[scene->light_free_count++] = handle.index;
	scene->light_count--;
}

internal u32
R_SceneLightCount(const R_Scene *scene)
{
	return scene->light_count;
}

internal R_LightSlot *
R_SceneLightGetSlot(R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->light_slots))
		return NULL;

	R_LightSlot *slot = &scene->light_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return NULL;

	return slot;
}

internal void
R_SceneLightSetPosition(R_Scene *scene, R_SceneHandle handle, v3 position)
{
	R_LightSlot *slot = R_SceneLightGetSlot(scene, handle);

	if (!slot)
		return;

	slot->light.position = position;
}

internal void
R_SceneLightSetColour(R_Scene *scene, R_SceneHandle handle, v3 colour)
{
	R_LightSlot *slot = R_SceneLightGetSlot(scene, handle);

	if (!slot)
		return;

	slot->light.colour = colour;
}

internal void
R_SceneLightSetIntensity(R_Scene *scene, R_SceneHandle handle, f32 intensity)
{
	R_LightSlot *slot = R_SceneLightGetSlot(scene, handle);

	if (!slot)
		return;

	slot->light.intensity = intensity;
}

internal b32
R_SceneLightHandleIsValid(const R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->light_slots))
		return false;

	const R_LightSlot *slot = &scene->light_slots[handle.index];
	return slot->active && slot->generation == handle.generation;
}

internal u32
R_SceneShadowCasterCount(const R_Scene *scene)
{
	return scene->shadow_caster_count;
}

internal const R_ShadowCaster *
R_SceneShadowCasterGet(const R_Scene *scene, u32 index)
{
	DebugLogAssert(scene->log_channel,
				   index < ArraySize(scene->shadow_casters),
				   "Out of range index (%u) into shadow caster array of size %llu",
				   index, ArraySize(scene->shadow_casters));
	
	return &scene->shadow_casters[index];
}

internal R_SceneHandle
R_SceneMaterialCreate(R_Scene *scene, const R_Material *material)
{
	DebugLogAssert(scene->log_channel,
				   scene->material_free_count > 0,
				   "Ran out of free material slots.");

	scene->material_free_count--;
	u32 slot_index = scene->material_free_list[scene->material_free_count];

	R_MaterialSlot *slot = &scene->material_slots[slot_index];

	slot->source = *material;
	slot->active = true;

	R_SceneMaterialBakeIntoGPU(scene, material, &scene->material_gpus[slot_index]);
	scene->material_buffer_dirty = true;

	scene->material_count++;

	R_SceneHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;

	return handle;
}

internal R_SceneHandle
R_SceneMaterialFromAssets(R_Scene *scene, const A_ModelMaterial *source)
{
	R_Material material = R_MaterialFromAsset(source, scene->assets);
	return R_SceneMaterialCreate(scene, &material);
}

internal void
R_SceneMaterialUpdate(R_Scene *scene, R_SceneHandle handle, const R_Material *material)
{
	if (handle.index >= ArraySize(scene->material_slots))
		return;

	R_MaterialSlot *slot = &scene->material_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return;

	slot->source = *material;

	R_SceneMaterialBakeIntoGPU(scene, material, &scene->material_gpus[handle.index]);
	scene->material_buffer_dirty = true;
}

internal void
R_SceneMaterialDestroy(R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->material_slots))
		return;

	R_MaterialSlot *slot = &scene->material_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return;

	slot->active = false;
	slot->generation++;

	MemZeroStruct(&scene->material_gpus[handle.index]);
	scene->material_buffer_dirty = true;

	scene->material_free_list[scene->material_free_count++] = handle.index;
	scene->material_count--;
}

internal u32
R_SceneMaterialCount(const R_Scene *scene)
{
	return scene->material_count;
}

internal const R_Material *
R_SceneMaterialGetSource(const R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->material_slots))
		return NULL;

	const R_MaterialSlot *slot = &scene->material_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return NULL;

	return &slot->source;
}

internal u64
R_SceneMaterialBufferAddr(const R_Scene *scene)
{
	return G_DeviceBufferAddress(scene->device, scene->material_buffer);
}

internal void
R_SceneMaterialBakeIntoGPU(const R_Scene *scene, const R_Material *material, R_GPU_Material *out)
{
	out->albedo_texture                       = R_SceneResolveTextureKey(scene, material->albedo_texture);
	out->normal_texture                       = R_SceneResolveTextureKey(scene, material->normal_texture);
	out->metallic_roughness_texture           = R_SceneResolveTextureKey(scene, material->metallic_roughness_texture);
	out->emissive_texture                     = R_SceneResolveTextureKey(scene, material->emissive_texture);
	out->occlusion_texture                    = R_SceneResolveTextureKey(scene, material->occlusion_texture);
	
	out->albedo_factor                        = material->albedo_factor;
	out->normal_scale                         = material->normal_scale;
	out->metallic_factor                      = material->metallic_factor;
	out->roughness_factor                     = material->roughness_factor;
	out->emissive_factor                      = material->emissive_factor;
	out->emissive_intensity                   = material->emissive_intensity;
	out->occlusion_intensity                  = material->occlusion_intensity;

	out->ior                                  = material->ior;
	
	out->transmission_texture                 = R_SceneResolveTextureKey(scene, material->transmission_texture);
	out->thickness_texture                    = R_SceneResolveTextureKey(scene, material->thickness_texture);

	out->transmission_factor                  = material->transmission_factor;
	out->thickness_factor                     = material->thickness_factor;

	out->attenuation_colour                   = material->attenuation_colour;
	out->attenuation_distance                 = material->attenuation_distance;

	out->specular_texture                     = R_SceneResolveTextureKey(scene, material->specular_texture);
	out->specular_colour_texture              = R_SceneResolveTextureKey(scene, material->specular_colour_texture);

	out->specular_factor                      = material->specular_factor;
	out->specular_colour_factor               = material->specular_colour_factor;

	out->clearcoat_texture                    = R_SceneResolveTextureKey(scene, material->clearcoat_texture);
	out->clearcoat_roughness_texture          = R_SceneResolveTextureKey(scene, material->clearcoat_roughness_texture);

	out->clearcoat_factor                     = material->clearcoat_factor;
	out->clearcoat_roughness_factor           = material->clearcoat_roughness_factor;

	out->sheen_colour_texture                 = R_SceneResolveTextureKey(scene, material->sheen_colour_texture);
	out->sheen_roughness_texture              = R_SceneResolveTextureKey(scene, material->sheen_roughness_texture);

	out->sheen_colour_factor                  = material->sheen_colour_factor;
	out->sheen_roughness_factor               = material->sheen_roughness_factor;
	
	out->iridescence_texture                  = R_SceneResolveTextureKey(scene, material->iridescence_texture);
	out->iridescence_thickness_texture        = R_SceneResolveTextureKey(scene, material->iridescence_thickness_texture);

	out->iridescence_factor                   = material->iridescence_factor;
	out->iridescence_ior                      = material->iridescence_ior;
	out->iridescence_thickness_min_nanometers = material->iridescence_thickness_min_nanometers;
	out->iridescence_thickness_max_nanometers = material->iridescence_thickness_max_nanometers;

	out->double_sided                         = material->double_sided;
	out->unlit                                = material->unlit;
	out->alpha_cutoff                         = material->alpha_cutoff;
	out->alpha_mode                           = material->alpha_mode;
}

internal b32
R_SceneMaterialHandleIsValid(const R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->material_slots))
		return false;

	const R_MaterialSlot *slot = &scene->material_slots[handle.index];
	return slot->active && slot->generation == handle.generation;
}

internal R_SceneHandle
R_SceneMeshCreate(R_Scene *scene, G_CmdBuffer *cmd, const R_MeshDesc *desc)
{
	DebugLogAssert(scene->log_channel,
				   scene->mesh_free_count > 0,
				   "Ran out of free mesh slots.");

	scene->mesh_free_count--;
	u32 slot_index = scene->mesh_free_list[scene->mesh_free_count];
	R_MeshSlot *slot = &scene->mesh_slots[slot_index];

	u32 page_index = R_SceneFindSuitablePage(scene, desc->vertex_count, desc->index_count);
	R_GeometryPage *page = &scene->geometry_pages[page_index];

	u64 vertex_offset = 0;
	u64 index_offset = 0;
	
	b32 vok = R_GeometryFreeListTryAlloc(&page->vertex_free, desc->vertex_count, &vertex_offset);
	b32 iok = R_GeometryFreeListTryAlloc(&page->index_free,  desc->index_count,  &index_offset);

	DebugLogAssert(scene->log_channel, vok, "Vertex region allocation failed after R_GeometryFreeListAvailable returned true.");
	DebugLogAssert(scene->log_channel, iok, "Index region allocation failed after R_GeometryFreeListAvailable returned true.");
	
	const u64 vertex_stride = sizeof(R_GPU_ModelVertex);
	const u64 index_stride  = sizeof(A_ModelIndex);

	G_BufferCopy vc = {0};
	vc.src_offset = 0;
	vc.dst_offset = vertex_offset * vertex_stride;
	vc.size = desc->vertex_count * vertex_stride;

	G_BufferCopy ic = {0};
	ic.src_offset = 0;
	ic.dst_offset = index_offset * index_stride;
	ic.size = desc->index_count * index_stride;
	
	G_CmdCopyBufferToBuffer(cmd, desc->vertex_buffer, page->vertex_buffer, 1, &vc);
	G_CmdCopyBufferToBuffer(cmd, desc->index_buffer,  page->index_buffer,  1, &ic);

	R_GPU_RenderMesh *gpu_mesh = &scene->mesh_gpus[slot_index];
	gpu_mesh->index_count = desc->index_count;
	gpu_mesh->first_index = index_offset;
	gpu_mesh->vertex_buffer = G_DeviceBufferAddress(scene->device, page->vertex_buffer) + (vertex_offset * sizeof(R_GPU_ModelVertex));

	if (!G_BufferKeyIsNull(desc->skin_buffer))
		gpu_mesh->skin_buffer = G_DeviceBufferAddress(scene->device, desc->skin_buffer);
	else
		gpu_mesh->skin_buffer = 0;
	
	page->vertex_count += desc->vertex_count;
	page->index_count  += desc->index_count;

	slot->page_index = page_index;
	slot->vertex_offset = vertex_offset;
	slot->vertex_count = desc->vertex_count;
	slot->index_offset = index_offset;
	slot->index_count = desc->index_count;
	slot->active = true;
	
	scene->mesh_count++;
	scene->mesh_buffer_dirty = true;

	R_SceneHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;
	
	return handle;
}

internal void
R_SceneMeshDestroy(R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->mesh_slots))
		return;

	R_MeshSlot *slot = &scene->mesh_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return;

	R_GeometryPage *page = &scene->geometry_pages[slot->page_index];

	R_GeometryFreeListRelease(&page->vertex_free, slot->vertex_offset, slot->vertex_count);
	R_GeometryFreeListRelease(&page->index_free,  slot->index_offset,  slot->index_count);

	slot->active = false;
	slot->generation++;

	MemZeroStruct(&scene->mesh_gpus[handle.index]);
	scene->mesh_buffer_dirty = true;

	scene->mesh_free_list[scene->mesh_free_count++] = handle.index;
	scene->mesh_count--;
}

internal u32
R_SceneMeshCount(const R_Scene *scene)
{
	return scene->mesh_count;
}

internal u64
R_SceneMeshBufferAddr(const R_Scene *scene)
{
	return G_DeviceBufferAddress(scene->device, scene->mesh_buffer);
}

internal b32
R_SceneMeshHandleIsValid(const R_Scene *scene, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(scene->mesh_slots))
		return false;

	const R_MeshSlot *slot = &scene->mesh_slots[handle.index];
	return slot->active && slot->generation == handle.generation;
}

internal void
R_SceneFlushMaterialBuffer(R_Scene *scene)
{
	G_DeviceBufferWrite(scene->device,
						  scene->material_buffer,
						  scene->material_gpus,
						  sizeof(scene->material_gpus), 0);
}

internal void
R_SceneFlushMeshBuffer(R_Scene *scene)
{
	G_DeviceBufferWrite(scene->device,
						  scene->mesh_buffer,
						  scene->mesh_gpus,
						  sizeof(scene->mesh_gpus), 0);
}

internal R_ModelImportReceipt
R_SceneImportModel(R_Scene *scene, G_CmdBuffer *cmd, Arena *arena, A_Handle handle, u32 max_count)
{
	A_ModelData *model_asset = &A_GetNow(scene->assets, handle)->model;
	
	u32 sub_model_count = model_asset->sub_model_count;
	const A_SubModel *sub_models = model_asset->sub_models;
	
	u32 actual_count = sub_model_count;
	
	if (sub_model_count > max_count)
	{
		DebugLogW(scene->log_channel,
				  "Hit max entry count! Truncating total sub model count %u down to %u.",
				  sub_model_count, max_count);

		actual_count = max_count;
	}
	
	R_ModelImportReceipt receipt = {0};
	receipt.count = sub_model_count;
	receipt.entries = ArenaPushArray(arena, R_ModelEntry, actual_count);

	for (u32 i = 0; i < actual_count; i++)
	{
		const A_SubModel *sub = &sub_models[i];
		R_ModelEntry *entry = &receipt.entries[i];

		R_MeshDesc mesh_desc = {0};
		mesh_desc.vertex_buffer = sub->vertex_buffer;
		mesh_desc.index_buffer = sub->index_buffer;
		mesh_desc.vertex_count = sub->vertex_count;
		mesh_desc.index_count = sub->index_count;
		mesh_desc.skin_buffer = sub->skin_buffer;

		entry->mesh = R_SceneMeshCreate(scene, cmd, &mesh_desc);

		entry->material = R_SceneMaterialFromAssets(scene, &sub->material);

		entry->transform = sub->transform;

		v3 centre = V3MulF32(V3Add(sub->bounds_min, sub->bounds_max), 0.5f);
		f32 radius = V3Length(V3Sub(sub->bounds_max, centre));

		entry->sphere_bounds = v4(centre.x, centre.y, centre.z, radius);

		entry->skin_index = sub->skin_index;
	}

	return receipt;
}

internal u32
R_SceneFindSuitablePage(R_Scene *scene, u32 vertex_count, u32 index_count)
{
	for (u32 i = 0 ; i < scene->geometry_page_count; i++)
	{
		R_GeometryPage *page = &scene->geometry_pages[i];

		if (R_GeometryFreeListAvailable(&page->vertex_free, vertex_count) &&
			R_GeometryFreeListAvailable(&page->index_free, index_count))
		{
			return i;
		}
	}
	
	DebugLogAssert(scene->log_channel,
				   scene->geometry_page_count < ArraySize(scene->geometry_pages),
				   "Exhausted all possible geometry pages.");

	u32 new_index = scene->geometry_page_count;

	scene->geometry_pages[new_index] = R_SceneCreateNewPage(scene);
	scene->geometry_page_count++;

	return new_index;
}

internal R_GeometryPage
R_SceneCreateNewPage(R_Scene *scene)
{
	DebugLogD(scene->log_channel, "Creating new geometry page...");

	// We use vertex pulling so don't need to use VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT.
	G_BufferAllocInfo vb_info = {0};
	vb_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		vb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vb_info.size = R_GEOMETRY_PAGE_VERTEX_BUFFER_SIZE;
 
	G_BufferAllocInfo ib_info = {0};
	ib_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	ib_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ib_info.size = R_GEOMETRY_PAGE_INDEX_BUFFER_SIZE;

	u32 max_vertices = vb_info.size / sizeof(R_GPU_ModelVertex);
	u32 max_indices  = ib_info.size / sizeof(A_ModelIndex);
	
	R_GeometryPage page = {0};
	page.vertex_buffer  = G_DeviceBufferAlloc(scene->device, &vb_info);
	page.index_buffer   = G_DeviceBufferAlloc(scene->device, &ib_info);
	page.vertex_count   = 0;
	page.index_count    = 0;
	page.max_vertices   = max_vertices;
	page.max_indices    = max_indices;

	R_GeometryFreeListInit(&page.vertex_free, max_vertices);
	R_GeometryFreeListInit(&page.index_free, max_indices);
 
	return page;
}

internal u32
R_ScenePageCount(const R_Scene *scene)
{
	return scene->geometry_page_count;
}

internal G_BindlessIndex
R_SceneResolveTextureKey(const R_Scene *scene, G_TextureKey key)
{
	if (G_TextureKeyIsNull(key))
		return G_BINDLESS_INDEX_INVALID;

	G_TextureViewKey view_key = G_DeviceTextureViewAuto(scene->device, key);
	return G_DeviceTextureViewBindless(scene->device, view_key);
}


internal R_EntityIterator R_EntityIteratorInit(R_Scene *scene)
{
	R_EntityIterator iter = {0};
	iter.scene = scene;
	iter.index = 0;

	return iter;
}

internal void R_EntityIteratorReset(R_EntityIterator *iter)
{
	iter->index = 0;
}

internal R_Entity *R_EntityIteratorNext(R_EntityIterator *iter, R_EntityType type)
{
	while (iter->index < DensePoolLiveCount(&iter->scene->entity_pool))
	{
		R_Entity *entity = &iter->scene->entities[iter->index++];

		if (entity->type == type)
			return entity;
	}

	return NULL;
}

internal void R_SceneInit(R_Scene *scene, Arena *arena, LOG_Channel log_channel)
{
	scene->log_channel = log_channel;

	DensePoolInit(&scene->entity_pool, arena, R_SCENE_MAX_ENTITIES);
	SlotPoolInit(&scene->mesh_pool, arena, R_SCENE_MAX_MESHES);
	SlotPoolInit(&scene->material_pool, arena, R_SCENE_MAX_MATERIALS);

	scene->frame_number = 0;
	scene->gpu_completed_frame = 0;
	scene->any_gpu_completed = false;
}

internal void R_SceneDestroy(R_Scene *scene)
{
	for (u32 i = 0; i < scene->geometry_page_count; i++)
	{
		G_BufferDestroy(scene->geometry_pages[i].vertex_buffer);
		G_BufferDestroy(scene->geometry_pages[i].index_buffer);
	}
}

internal void R_SceneReleaseRegion(R_Scene *scene, const R_MeshAllocRegion *region)
{
	R_GeometryPage *page = &scene->geometry_pages[region->page_index];

	R_GeometryFreeListRelease(&page->vertex_free, region->vertex_offset, region->vertex_count);
	R_GeometryFreeListRelease(&page->index_free, region->index_offset, region->index_count);

	page->vertex_count -= region->vertex_count;
	page->index_count -= region->index_count;
}

internal void R_SceneBeginFrame(R_Scene *scene, u64 frame_number,
								b32 has_completed, u64 gpu_completed_frame)
{
	scene->frame_number = frame_number;
	scene->any_gpu_completed = has_completed;
	scene->gpu_completed_frame = gpu_completed_frame;

	if (!has_completed)
		return;
	
	u32 kept = 0;

	for (u32 i = 0; i < scene->retired_region_count; i++)
	{
		R_SceneRetiredRegion *r = &scene->retired_regions[i];

		if (r->retire_frame <= gpu_completed_frame)
			R_SceneReleaseRegion(scene, &r->region);
		else
			scene->retired_regions[kept++] = *r;
	}

	scene->retired_region_count = kept;
}

internal R_EntityHandle R_SceneEntityCreate(R_Scene *scene, R_EntityType type)
{
	R_EntityHandle handle = {0};
	handle.id = DensePoolGetStableID(&scene->entity_pool);
	handle.type = type;

	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);

	MemZeroStruct(&scene->entities[index]);
	scene->entities[index].type = type;
	scene->entity_count[type]++;
	
	return handle;
}

internal void R_SceneEntityDestroy(R_Scene *scene, R_EntityHandle handle)
{
	u32 dense = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	u32 moved = DensePoolFreeID(&scene->entity_pool, handle.id);

	scene->entities[dense] = scene->entities[moved];
	scene->entity_count[handle.type]--;
}

internal void R_SceneSetObjectTransform(R_Scene *scene, R_EntityHandle handle, m4 transform)
{
	DebugLogAssert(scene->log_channel, handle.type == R_EntityType_Object, "Entity handle must point to object.");
	
	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	R_Object *object = &scene->entities[index].object;

	if (!object->has_transform)
	{
		object->prev_transform = transform;
		object->has_transform = true;
	}
	
	object->transform = transform;
	object->normal_matrix = M4RemoveTranslation(M4Inverse(M4Transpose(transform)));
}

internal void R_SceneTeleportObject(R_Scene *scene, R_EntityHandle handle, m4 transform)
{
	DebugLogAssert(scene->log_channel, handle.type == R_EntityType_Object, "Entity handle must point to object.");
	
	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	R_Object *object = &scene->entities[index].object;

	object->transform = transform;
	object->prev_transform = transform;
	object->has_transform = true;
	object->normal_matrix = M4RemoveTranslation(M4Inverse(M4Transpose(transform)));
}

internal void R_SceneBeginSimSteps(R_Scene *scene, u32 step_count)
{
	if (step_count == 0)
		return;
	
	for (u32 i = 0; i < DensePoolLiveCount(&scene->entity_pool); i++)
	{
		R_Entity *entity = &scene->entities[i];

		if (entity->type == R_EntityType_Object)
			entity->object.prev_transform = entity->object.transform;
	}

	scene->last_sim_step_count = step_count;
}

internal m4 R_SceneObjectRenderTransform(const R_Scene *scene, R_EntityHandle handle, f32 interp_t)
{
	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	const R_Object *object = &scene->entities[index].object;
	return M4Lerp(object->prev_transform, object->transform, interp_t);
}

internal f32 R_SceneInterpolationFactor(const R_Scene *scene, f32 alpha)
{
	u32 k = scene->last_sim_step_count;

	if (k == 0)
		return 1.f;

	f32 t = ((f32)k - 1.f + alpha) / (f32)k;
	return ClampValue(t, 0.f, 1.f);
}

internal void R_SceneSetObjectLocalSphereBounds(R_Scene *scene, R_EntityHandle handle, v4 local_sphere_bounds)
{
	DebugLogAssert(scene->log_channel, handle.type == R_EntityType_Object, "Entity handle must point to object.");
	
	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	
	scene->entities[index].object.local_sphere_bounds = local_sphere_bounds;
}

internal void R_SceneSetObjectMesh(R_Scene *scene, R_EntityHandle handle, R_MeshHandle mesh)
{
	DebugLogAssert(scene->log_channel, handle.type == R_EntityType_Object, "Entity handle must point to object.");
	
	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	
	scene->entities[index].object.page_index = mesh.page_index;
	scene->entities[index].object.mesh_index = mesh.slot_index;
}

internal void R_SceneSetObjectMaterial(R_Scene *scene, R_EntityHandle handle, R_MaterialHandle material)
{
	DebugLogAssert(scene->log_channel, handle.type == R_EntityType_Object, "Entity handle must point to object.");
	
	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	
	scene->entities[index].object.material_index = material.index;
}

internal void R_SceneSetObjectSkinning(R_Scene *scene, R_EntityHandle handle, const m4 *palette, u32 joint_count)
{
	DebugLogAssert(scene->log_channel, handle.type == R_EntityType_Object, "Entity handle must point to object.");
	
	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	
	scene->entities[index].object.skinning_palette = palette;
	scene->entities[index].object.skinning_joint_count = joint_count;
}

internal void R_SceneSetLightParam(R_Scene *scene, R_EntityHandle handle, R_Light light)
{
	DebugLogAssert(scene->log_channel, handle.type == R_EntityType_Light, "Entity handle must point to light.");
	
	u32 index = DensePoolIndexFromID(&scene->entity_pool, handle.id);
	
	scene->entities[index].light = light;
}

internal R_MeshHandle R_SceneAllocMesh(R_Scene *scene, const R_MeshDesc *desc)
{
	u32 slot_index = 0;

	if (!SlotPoolAlloc(&scene->mesh_pool, &slot_index))
	{
		DebugLogE(scene->log_channel, "Unable to allocate more meshes!");
		return R_MeshHandleNull();
	}

	u32 page_index = R_SceneFindSuitablePage(scene,
											 desc->vertex_count, desc->vertex_stride,
											 desc->index_count, desc->index_type);
	
	R_GeometryPage *page = &scene->geometry_pages[page_index];

	u64 vertex_offset = 0;
	u64 index_offset = 0;
	
	b32 vok = R_GeometryFreeListTryAlloc(&page->vertex_free, desc->vertex_count, &vertex_offset);
	b32 iok = R_GeometryFreeListTryAlloc(&page->index_free,  desc->index_count,  &index_offset);

	DebugLogAssert(scene->log_channel, vok, "Vertex region allocation failed after R_GeometryFreeListAvailable returned true.");
	DebugLogAssert(scene->log_channel, iok, "Index region allocation failed after R_GeometryFreeListAvailable returned true.");
	
	const u64 index_stride = G_IndexTypeStride(desc->index_type);

	// The region was just taken off the free list, so no recorded frame can
	// be reading it (freed regions are retired, see R_SceneFreeMesh). Write
	// straight into the mapped page; the next queue submit makes the host
	// writes visible to the GPU.
	{
		u8 *vertices_mapped = G_BufferMap(page->vertex_buffer);
		u8 *indices_mapped = G_BufferMap(page->index_buffer);

		MemCopy(vertices_mapped + vertex_offset * desc->vertex_stride, desc->vertices, desc->vertex_count * desc->vertex_stride);
		MemCopy(indices_mapped + index_offset * index_stride, desc->indices, desc->index_count * index_stride);
	}
	
	R_GPU_RenderMesh *gpu_mesh = &scene->gpu_meshes[slot_index];
	gpu_mesh->index_count = desc->index_count;
	gpu_mesh->first_index = index_offset;
	gpu_mesh->vertex_buffer = G_BufferAddress(page->vertex_buffer) + (vertex_offset * desc->vertex_stride);

	if (!G_ResourceKeyIsNull(desc->skin_buffer))
		gpu_mesh->skin_buffer = G_BufferAddress(desc->skin_buffer);
	else
		gpu_mesh->skin_buffer = 0;
	
	page->vertex_count += desc->vertex_count;
	page->index_count += desc->index_count;

	R_MeshAllocRegion *alloc = &scene->mesh_allocs[slot_index];
	alloc->page_index = page_index;
	alloc->vertex_offset = vertex_offset;
	alloc->vertex_count = desc->vertex_count;
	alloc->index_offset = index_offset;
	alloc->index_count = desc->index_count;

	scene->mesh_high_water = MaxValue(scene->mesh_high_water, slot_index + 1);
	
	R_MeshHandle handle = {0};
	handle.slot_index = slot_index;
	handle.page_index = page_index;

	return handle;
}

internal void R_SceneFreeMesh(R_Scene *scene, R_MeshHandle handle)
{
	// The table slot can be reused straight away (every packet carries its
	// own copy of the mesh table), but the page region can't: frames
	// already handed to the render side may still draw it.
	DebugLogAssert(scene->log_channel,
				   scene->retired_region_count < ArraySize(scene->retired_regions),
				   "Too many mesh regions waiting to retire.");

	R_SceneRetiredRegion *retired = &scene->retired_regions[scene->retired_region_count++];
	retired->region = scene->mesh_allocs[handle.slot_index];
	retired->retire_frame = scene->frame_number;

	MemZeroStruct(&scene->gpu_meshes[handle.slot_index]);
	MemZeroStruct(&scene->mesh_allocs[handle.slot_index]);
	
	SlotPoolFree(&scene->mesh_pool, handle.slot_index);
}

internal u32 R_SceneCountOfMeshes(const R_Scene *scene)
{
	return SlotPoolLiveCount(&scene->mesh_pool);
}

internal u32 R_SceneFindSuitablePage(R_Scene *scene,
									 u32 vertex_count, u64 vertex_stride,
									 u32 index_count, G_IndexType index_type)
{
	for (u32 i = 0; i < scene->geometry_page_count; i++)
	{
		R_GeometryPage *page = &scene->geometry_pages[i];
		
		if (R_GeometryFreeListHasAvailable(&page->vertex_free, vertex_count) &&
			R_GeometryFreeListHasAvailable(&page->index_free, index_count) &&
			page->vertex_stride == vertex_stride &&
			page->index_type == index_type)
		{
			return i;
		}
	}
	
	DebugLogAssert(scene->log_channel,
				   scene->geometry_page_count < ArraySize(scene->geometry_pages),
				   "Exhausted all possible geometry pages.");

	u32 new_index = scene->geometry_page_count;

	scene->geometry_pages[new_index] = R_SceneCreateNewPage(scene, vertex_stride, index_type);
	scene->geometry_page_count++;

	return new_index;
}

internal R_GeometryPage R_SceneCreateNewPage(R_Scene *scene, u64 vertex_stride, G_IndexType index_type)
{
	DebugLogD(scene->log_channel, "Creating a new geometry page... VERTEXSTRIDE: %llu, INDEXTYPE: %u", vertex_stride, index_type);

	// We use vertex pulling so don't need to use VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT.
	G_BufferAllocInfo vb_info = {0};
	vb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vb_info.size = R_GEOMETRY_PAGE_VERTEX_BUFFER_SIZE;
	vb_info.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	vb_info.usage |= VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	//vb_info.usage |= VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
 
	G_BufferAllocInfo ib_info = {0};
	ib_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ib_info.size = R_GEOMETRY_PAGE_INDEX_BUFFER_SIZE;
	ib_info.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	ib_info.usage |= VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	ib_info.usage |= VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT;
	//ib_info.usage |= K_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KH;

	u64 index_stride = G_IndexTypeStride(index_type);
	
	u32 max_vertices = vb_info.size / vertex_stride;
	u32 max_indices  = ib_info.size / index_stride;
	
	R_GeometryPage page = {0};
	page.vertex_buffer = G_BufferAlloc(&vb_info);
	page.index_buffer = G_BufferAlloc(&ib_info);
	page.vertex_count = 0;
	page.index_count = 0;
	page.vertex_stride = vertex_stride;
	page.index_type = index_type;
	page.max_vertices = max_vertices;
	page.max_indices = max_indices;

	R_GeometryFreeListInit(&page.vertex_free, max_vertices);
	R_GeometryFreeListInit(&page.index_free, max_indices);
 
	return page;
}

internal u32 R_ScenePageCount(const R_Scene *scene)
{
	return scene->geometry_page_count;
}

internal R_MaterialHandle R_SceneAddMaterial(R_Scene *scene, const R_Material *material)
{
	u32 slot_index = 0;

	if (!SlotPoolAlloc(&scene->material_pool, &slot_index))
	{
		DebugLogE(scene->log_channel, "Unable to allocate more materials!");
		return R_MaterialHandleNull();
	}

	scene->cpu_materials[slot_index] = *material;
	
	R_SceneBakeMaterialIntoGPU(scene, material, &scene->gpu_materials[slot_index]);
	scene->material_high_water = MaxValue(scene->material_high_water, slot_index + 1);

	R_MaterialHandle handle = {0};
	handle.index = slot_index;

	return handle;
}

internal R_MaterialHandle R_SceneAddMaterialFromAssets(R_Scene *scene, const A_ModelMaterial *source)
{
	R_Material render_material = R_MaterialFromAsset(source);
	return R_SceneAddMaterial(scene, &render_material);
}

internal void R_SceneUpdateMaterial(R_Scene *scene, R_MaterialHandle handle, const R_Material *material)
{
	scene->cpu_materials[handle.index] = *material;
	R_SceneBakeMaterialIntoGPU(scene, material, &scene->gpu_materials[handle.index]);
}

internal void R_SceneFreeMaterial(R_Scene *scene, R_MaterialHandle handle)
{
	MemZeroStruct(&scene->gpu_materials[handle.index]);
	SlotPoolFree(&scene->material_pool, handle.index);
}

internal u32 R_SceneCountOfMaterials(const R_Scene *scene)
{
	return SlotPoolLiveCount(&scene->material_pool);
}

internal const R_Material *R_SceneGetMaterial(const R_Scene *scene, R_MaterialHandle handle)
{
	return &scene->cpu_materials[handle.index];
}

internal void R_SceneBakeMaterialIntoGPU(const R_Scene *scene, const R_Material *material, R_GPU_Material *out)
{
	out->albedo_texture                       = R_SceneResolveToBindlessIndex(scene, material->albedo_texture);
	out->normal_texture                       = R_SceneResolveToBindlessIndex(scene, material->normal_texture);
	out->metallic_roughness_texture           = R_SceneResolveToBindlessIndex(scene, material->metallic_roughness_texture);
	out->emissive_texture                     = R_SceneResolveToBindlessIndex(scene, material->emissive_texture);
	out->occlusion_texture                    = R_SceneResolveToBindlessIndex(scene, material->occlusion_texture);
	
	out->albedo_factor                        = material->albedo_factor;
	out->normal_scale                         = material->normal_scale;
	out->metallic_factor                      = material->metallic_factor;
	out->roughness_factor                     = material->roughness_factor;
	out->emissive_factor                      = material->emissive_factor;
	out->emissive_intensity                   = material->emissive_intensity;
	out->occlusion_intensity                  = material->occlusion_intensity;

	out->ior                                  = material->ior;
	
	out->transmission_texture                 = R_SceneResolveToBindlessIndex(scene, material->transmission_texture);
	out->thickness_texture                    = R_SceneResolveToBindlessIndex(scene, material->thickness_texture);

	out->transmission_factor                  = material->transmission_factor;
	out->thickness_factor                     = material->thickness_factor;

	out->attenuation_colour                   = material->attenuation_colour;
	out->attenuation_distance                 = material->attenuation_distance;

	out->specular_texture                     = R_SceneResolveToBindlessIndex(scene, material->specular_texture);
	out->specular_colour_texture              = R_SceneResolveToBindlessIndex(scene, material->specular_colour_texture);

	out->specular_factor                      = material->specular_factor;
	out->specular_colour_factor               = material->specular_colour_factor;

	out->clearcoat_texture                    = R_SceneResolveToBindlessIndex(scene, material->clearcoat_texture);
	out->clearcoat_roughness_texture          = R_SceneResolveToBindlessIndex(scene, material->clearcoat_roughness_texture);

	out->clearcoat_factor                     = material->clearcoat_factor;
	out->clearcoat_roughness_factor           = material->clearcoat_roughness_factor;

	out->sheen_colour_texture                 = R_SceneResolveToBindlessIndex(scene, material->sheen_colour_texture);
	out->sheen_roughness_texture              = R_SceneResolveToBindlessIndex(scene, material->sheen_roughness_texture);

	out->sheen_colour_factor                  = material->sheen_colour_factor;
	out->sheen_roughness_factor               = material->sheen_roughness_factor;
	
	out->iridescence_texture                  = R_SceneResolveToBindlessIndex(scene, material->iridescence_texture);
	out->iridescence_thickness_texture        = R_SceneResolveToBindlessIndex(scene, material->iridescence_thickness_texture);

	out->iridescence_factor                   = material->iridescence_factor;
	out->iridescence_ior                      = material->iridescence_ior;
	out->iridescence_thickness_min_nanometers = material->iridescence_thickness_min_nanometers;
	out->iridescence_thickness_max_nanometers = material->iridescence_thickness_max_nanometers;

	out->double_sided                         = material->double_sided;
	out->unlit                                = material->unlit;
	out->alpha_cutoff                         = material->alpha_cutoff;
	out->alpha_mode                           = material->alpha_mode;
}

internal u32 R_SceneResolveToBindlessIndex(const R_Scene *scene, G_ResourceKey key)
{
	if (G_ResourceKeyIsNull(key))
		return G_BINDLESS_INDEX_INVALID;
	else
		return G_TextureViewBindless(G_TextureViewAuto(key));
}

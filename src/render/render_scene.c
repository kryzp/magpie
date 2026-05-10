
internal void
R_SceneInit(R_Scene *scene, Arena *arena, GFX_Device *device, LOG_Channel log_channel)
{
	MemZeroStruct(scene);

	scene->arena = arena;
	scene->device = device;
	scene->log_channel = log_channel;

	// Hook up the free lists.
	for (u32 i = R_SCENE_MAX_OBJECTS; i > 0; i--)
		scene->object_free_list[scene->object_free_count++] = i - 1;

	for (u32 i = R_SCENE_MAX_LIGHTS; i > 0; i--)
		scene->light_free_list[scene->light_free_count++] = i - 1;

	GFX_BufferAllocInfo mesh_buffer_info = {0};
	mesh_buffer_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	mesh_buffer_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	mesh_buffer_info.size  = sizeof(R_GPU_RenderMesh) * R_SCENE_MAX_MESHES;
 
	GFX_BufferAllocInfo material_buffer_info = {0};
	material_buffer_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	material_buffer_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	material_buffer_info.size  = sizeof(R_GPU_Material) * R_SCENE_MAX_MATERIALS;
 
	scene->mesh_buffer	   = GFX_DeviceBufferAlloc(device, &mesh_buffer_info);
	scene->material_buffer = GFX_DeviceBufferAlloc(device, &material_buffer_info);
 
	scene->mesh_buffer_dirty	 = true;
	scene->material_buffer_dirty = true;
	
	DebugLogI(scene->log_channel, "Scene Initialized.");
}

internal void
R_SceneDestroy(R_Scene *scene)
{
	GFX_DeviceBufferDestroy(scene->device, scene->mesh_buffer);
	GFX_DeviceBufferDestroy(scene->device, scene->material_buffer);
 
	for (R_GeometryPage *page = scene->geometry_page_head; page; page = page->next)
	{
		GFX_DeviceBufferDestroy(scene->device, page->vertex_buffer);
		GFX_DeviceBufferDestroy(scene->device, page->index_buffer);
	}
	
	DebugLogI(scene->log_channel, "Scene Destroyed.");
}

internal void
R_SceneDebug(const R_Scene *scene)
{
	// TODO
}

internal R_SceneResources
R_SceneRefreshTransientResources(R_Scene *scene, GFX_RingBuffer *ring)
{
	R_SceneResources resources = {0};
 
	R_SceneUpdatePageBuffer(scene, ring, &resources);
 
	if (scene->object_count > 0)
		R_SceneUpdateObjectBuffer(scene, ring, &resources);
 
	if (scene->light_count > 0)
		R_SceneUpdateLightBuffer(scene, ring, &resources);
 
	if (scene->mesh_count > 0 && scene->mesh_buffer_dirty)
	{
		R_SceneUpdateMeshBuffer(scene);
		scene->mesh_buffer_dirty = false;
	}
 
	if (scene->material_count > 0 && scene->material_buffer_dirty)
	{
		R_SceneUpdateMaterialBuffer(scene);
		scene->material_buffer_dirty = false;
	}
 
	return resources;
}

internal void
R_SceneUpdateObjectBuffer(R_Scene *scene, GFX_RingBuffer *ring, R_SceneResources *out)
{
	out->object_buffer = GFX_RingBufferPushArray(ring, R_GPU_ObjectData, scene->object_count);
 
	R_GPU_ObjectData *mapped = out->object_buffer.cpu;

	u32 write_index = 0;
 
	// TODO: The list is sparse so this is just gonna be
	//       slower than it really should be...
	
	for (u32 i = 0; i < R_SCENE_MAX_OBJECTS && write_index < scene->object_count; i++)
	{
		R_SceneObjectSlot *slot = &scene->object_slots[i];
		
		if (!slot->active)
			continue;
 
		const R_Object *obj = &slot->object;
 
		mapped[write_index].model_matrix   = obj->transform;
		mapped[write_index].normal_matrix  = M4RemoveTranslation(M4Inverse(M4Transpose(obj->transform)));
		mapped[write_index].sphere_bounds  = obj->sphere_bounds;
		mapped[write_index].material_index = obj->material.value;
		mapped[write_index].mesh_index	   = obj->mesh.value;
		mapped[write_index].page_index	   = slot->page_index;
 
		write_index++;
	}
}

internal void
R_SceneUpdateLightBuffer(R_Scene *scene, GFX_RingBuffer *ring, R_SceneResources *out)
{
	scene->shadow_caster_count = 0;

	out->light_buffer = GFX_RingBufferPushArray(ring, R_GPU_Light, scene->light_count);

	R_GPU_Light *mapped = out->light_buffer.cpu;

	u32 write_index = 0;
	i32 shadow_slot_index = 0;

	for (u32 i = 0; i < R_SCENE_MAX_LIGHTS && write_index < scene->light_count; i++)
	{
		R_SceneLightSlot *slot = &scene->light_slots[i];

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

internal void
R_SceneUpdatePageBuffer(R_Scene *scene, GFX_RingBuffer *ring, R_SceneResources *out)
{
	// geometry_page_count may be 0 on the first frame before any mesh is
	// registered. Push at least 1 entry to avoid a zero-size allocation.
	u32 count = scene->geometry_page_count > 0 ? scene->geometry_page_count : 1;
 
	out->page_table_buffer = GFX_RingBufferPushArray(ring, R_GPU_PagePointers, count);
 
	R_GPU_PagePointers *mapped	= out->page_table_buffer.cpu;
	R_GeometryPage	   *page	= scene->geometry_page_head;
 
	for (u32 i = 0; i < scene->geometry_page_count; i++, page = page->next)
		mapped[i].vertex_buffer = GFX_DeviceBufferAddress(scene->device, page->vertex_buffer);
}

internal void
R_SceneDrawIndirect(const R_Scene *scene,
					GFX_CmdBuffer *cmd,
					GFX_BufferKey indirect_buffer,
					GFX_BufferKey count_buffer)
{
	for (const R_GeometryPage *page = scene->geometry_page_head; page; page = page->next)
	{
		GFX_CmdBindIndexBuffer(cmd,
							   page->index_buffer,
							   0, VK_WHOLE_SIZE,
							   VK_INDEX_TYPE_UINT32);
		
		GFX_CmdDrawIndexedIndirectCount(cmd,
										indirect_buffer, 0,
										count_buffer, 0,
										R_SCENE_MAX_OBJECTS,
										sizeof(R_GPU_IndirectDraw));
	}
}

internal void
R_SceneUpdateMeshBuffer(R_Scene *scene)
{
	AssertTrue(scene->mesh_count <= R_SCENE_MAX_MESHES);
 
	GFX_DeviceBufferWrite(scene->device,
						  scene->mesh_buffer,
						  scene->gpu_meshes,
						  sizeof(R_GPU_RenderMesh) * scene->mesh_count, 0);
}

internal void
R_SceneUpdateMaterialBuffer(R_Scene *scene)
{
	AssertTrue(scene->material_count <= R_SCENE_MAX_MATERIALS);
 
	GFX_DeviceBufferWrite(scene->device,
						  scene->material_buffer,
						  scene->gpu_materials,
						  sizeof(R_GPU_Material) * scene->material_count, 0);
}

internal u32
R_SceneFindSuitablePage(R_Scene *scene, u32 vertex_count, u32 index_count)
{
	if (scene->geometry_page_head)
	{
		// Walk to the last page.
		// The page count should be small
		// 'cuz each page is pretty big.
		R_GeometryPage *last = scene->geometry_page_head;
		u32 last_index = 0;
		u32 walk_index = 0;

		for (R_GeometryPage *p = scene->geometry_page_head; p; p = p->next, walk_index++)
		{
			last = p;
			last_index = walk_index;
		}

		b32 fits = ((last->vertex_count + vertex_count) <= last->max_vertices &&
					(last->index_count  + index_count)  <= last->max_indices);

		if (fits)
			return last_index;
	}

	R_GeometryPage *new_page = ArenaPushArray(scene->arena, R_GeometryPage, 1);
	*new_page = R_SceneCreateNewPage(scene);

	u32 new_index = scene->geometry_page_count;

	if (scene->geometry_page_head)
	{
		R_GeometryPage *tail = scene->geometry_page_head;

		while (tail->next)
			tail = tail->next;

		tail->next = new_page;
	}
	else
	{
		scene->geometry_page_head = new_page;
	}

	scene->geometry_page_count++;

	return new_index;
}

internal R_GeometryPage
R_SceneCreateNewPage(R_Scene *scene)
{
	// We use vertex pulling so don't need to use VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT.
	GFX_BufferAllocInfo vb_info = {0};

	vb_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	
	vb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vb_info.size  = R_PAGE_VERTEX_BUFFER_SIZE;
 
	GFX_BufferAllocInfo ib_info = {0};

	ib_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

	ib_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ib_info.size  = R_PAGE_INDEX_BUFFER_SIZE;
 
	R_GeometryPage page = {0};

	page.next = NULL;
	
	page.vertex_buffer  = GFX_DeviceBufferAlloc(scene->device, &vb_info);
	page.index_buffer   = GFX_DeviceBufferAlloc(scene->device, &ib_info);

	page.vertex_count   = 0;
	page.index_count    = 0;

	page.max_vertices   = R_PAGE_VERTEX_BUFFER_SIZE / sizeof(R_GPU_ModelVertex);
	page.max_indices    = R_PAGE_INDEX_BUFFER_SIZE  / sizeof(AST_ModelIndex);
 
	return page;
}

internal R_SceneObjectSlot *
R_SceneObjectGetSlot(R_Scene *scene, R_SceneObjectHandle handle)
{
	if (handle.index >= R_SCENE_MAX_OBJECTS)
		return NULL;

	R_SceneObjectSlot *slot = &scene->object_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return NULL;

	return slot;
}

internal R_SceneObjectHandle
R_SceneObjectCreate(R_Scene *scene, const R_Object *object)
{
	AssertTrue(scene->object_free_count > 0);

	u32 slot_index = scene->object_free_list[--scene->object_free_count];

	R_SceneObjectSlot *slot = &scene->object_slots[slot_index];

	u32 mesh_handle = object->mesh.value;
	
	AssertTrue(mesh_handle < scene->mesh_count);

	slot->object = *object;
	slot->page_index = scene->mesh_registry[mesh_handle].page;
	slot->active = true;

	scene->object_count++;

	R_SceneObjectHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;

	return handle;
}

internal void
R_SceneObjectRemove(R_Scene *scene, R_SceneObjectHandle handle)
{
	R_SceneObjectSlot *slot = R_SceneObjectGetSlot(scene, handle);

	if (!slot)
		return;

	slot->active = false;
	slot->generation++; // Invalidate outstanding handles.

	scene->object_free_list[scene->object_free_count] = handle.index;
	scene->object_free_count++;

	scene->object_count--;
}

internal void
R_SceneObjectSetTransform(R_Scene *scene, R_SceneObjectHandle handle, m4 transform)
{
	R_SceneObjectSlot *slot = R_SceneObjectGetSlot(scene, handle);

	if (!slot)
		return;

	slot->object.transform = transform;
}

internal u32
R_SceneGetObjectCount(const R_Scene *scene)
{
	return scene->object_count;
}

internal R_SceneLightSlot *
R_SceneLightGetSlot(R_Scene *scene, R_SceneLightHandle handle)
{
	if (handle.index >= R_SCENE_MAX_LIGHTS)
		return NULL;

	R_SceneLightSlot *slot = &scene->light_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return NULL;

	return slot;
}

internal R_SceneLightHandle
R_SceneLightCreate(R_Scene *scene, const R_Light *light)
{
	AssertTrue(scene->light_free_count > 0);
 
	u32 slot_index = scene->light_free_list[--scene->light_free_count];
 
	R_SceneLightSlot *slot = &scene->light_slots[slot_index];
	slot->light = *light;
	slot->active = true;
 
	scene->light_count++;
 
	R_SceneLightHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;
	
	return handle;
}

internal void
R_SceneLightRemove(R_Scene *scene, R_SceneLightHandle handle)
{
	R_SceneLightSlot *slot = R_SceneLightGetSlot(scene, handle);

	if (!slot)
		return;

	slot->active = false;
	slot->generation++; // Invalidate outstanding handles.

	scene->light_free_list[scene->light_free_count] = handle.index;
	scene->light_free_count++;

	scene->light_count--;
}

internal void
R_SceneLightSetPosition(R_Scene *scene, R_SceneLightHandle handle, v3 position)
{
	R_SceneLightSlot *slot = R_SceneLightGetSlot(scene, handle);

	if (!slot)
		return;

	slot->light.position = position;
}

internal void
R_SceneLightSetColour(R_Scene *scene, R_SceneLightHandle handle, v3 colour)
{
	R_SceneLightSlot *slot = R_SceneLightGetSlot(scene, handle);

	if (!slot)
		return;

	slot->light.colour = colour;
}

internal void
R_SceneLightSetIntensity(R_Scene *scene, R_SceneLightHandle handle, f32 intensity)
{
	R_SceneLightSlot *slot = R_SceneLightGetSlot(scene, handle);

	if (!slot)
		return;

	slot->light.intensity = intensity;
}

internal u32
R_SceneGetLightCount(const R_Scene *scene)
{
	return scene->light_count;
}

internal const R_ShadowCaster *
R_SceneShadowCasterGet(const R_Scene *scene, u32 i)
{
	AssertTrue(i < scene->shadow_caster_count);
	return &scene->shadow_casters[i];
}

internal u32
R_SceneGetShadowCasterCount(const R_Scene *scene)
{
	return scene->shadow_caster_count;
}

internal R_SceneRegisterModelReceipt
R_SceneRegisterModel(R_Scene *scene,
					 Arena *arena,
					 AST_Assets *assets,
					 AST_Handle model_handle,
					 u32 max_entries)
{
	AST_Asset *model_asset = AST_GetNow(assets, model_handle, AST_Type_Model);
	
	u32 sub_model_count = model_asset->model.sub_model_count;
	const AST_SubModel *sub_models = model_asset->model.sub_models;

	R_SceneRegisterModelReceipt receipt = {0};
	receipt.entry_count = MinValue(sub_model_count, max_entries);
	receipt.entries = ArenaPushArray(arena, R_SceneModelEntry, receipt.entry_count);
	
	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(scene->device);
	
	for (u32 i = 0; i < receipt.entry_count; i++)
	{
		const AST_SubModel *src = &sub_models[i];

		receipt.entries[i].mesh = R_SceneRegisterMeshFromBuffers(scene, &cmd,
																 src->vertex_buffer,
																 src->index_buffer,
																 src->vertex_count,
																 src->index_count);

		receipt.entries[i].material = R_SceneRegisterMaterial(scene, &src->material, assets);
		
		receipt.entries[i].transform = src->transform;

		v3  centre = V3MulF32(V3Add(src->bounds_min, src->bounds_max), 0.5f);
		f32 radius = V3Length(V3Sub(src->bounds_max, centre));

		receipt.entries[i].sphere_bounds = v4(centre.x, centre.y, centre.z, radius);
	}

	GFX_DeviceSubmitImEnd(scene->device, &cmd);
	
	return receipt;
}

internal R_SceneMeshHandle
R_SceneRegisterMeshFromBuffers(R_Scene *scene,
							   const GFX_CmdBuffer *cmd,
							   GFX_BufferKey vertex_buffer,
							   GFX_BufferKey index_buffer,
							   u32 vertex_count,
							   u32 index_count)
{
	AssertTrue(scene->mesh_count < R_SCENE_MAX_MESHES);

	u32 page_index = R_SceneFindSuitablePage(scene, vertex_count, index_count);

	R_GeometryPage *page = scene->geometry_page_head;
	for (u32 i = 0; i < page_index; i++, page = page->next); // get page_index'th page

	const u64 vertex_stride = sizeof(R_GPU_ModelVertex);
	const u64 index_stride  = sizeof(AST_ModelIndex);

	GFX_BufferCopy vc = {0};
	vc.src_offset = 0;
	vc.dst_offset = page->vertex_count * vertex_stride;
	vc.size = vertex_count * vertex_stride;

	GFX_BufferCopy ic = {0};
	ic.src_offset = 0;
	ic.dst_offset = page->index_count * index_stride;
	ic.size = index_count * index_stride;

	GFX_CmdCopyBufferToBuffer(cmd, vertex_buffer, page->vertex_buffer, 1, &vc);
	GFX_CmdCopyBufferToBuffer(cmd, index_buffer,  page->index_buffer,  1, &ic);

	u32 mesh_data_index = scene->mesh_count;

	R_GPU_RenderMesh *gpu_mesh = &scene->gpu_meshes[mesh_data_index];
	gpu_mesh->index_count = index_count;
	gpu_mesh->first_index = page->index_count;
	gpu_mesh->vertex_buffer = GFX_DeviceBufferAddress(scene->device, page->vertex_buffer) + (page->vertex_count * sizeof(R_GPU_ModelVertex));

	page->vertex_count += vertex_count;
	page->index_count  += index_count;

	scene->mesh_registry[mesh_data_index].page  = page_index;
	scene->mesh_registry[mesh_data_index].index = mesh_data_index;

	scene->mesh_count++;
	scene->mesh_buffer_dirty = true;

	R_SceneMeshHandle handle = {0};
	handle.value = mesh_data_index;

	DebugLogD(scene->log_channel, "Registered Mesh.");

	return handle;
}

internal R_SceneMeshHandle
R_SceneRegisterMesh(R_Scene *scene, const R_Mesh *mesh)
{
	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(scene->device);

	R_SceneMeshHandle h = R_SceneRegisterMeshFromBuffers(scene, &cmd,
														 mesh->vertex_buffer,
														 mesh->index_buffer,
														 mesh->vertex_count,
														 mesh->index_count);

	GFX_DeviceSubmitImEnd(scene->device, &cmd);

	return h;
}

internal u64
R_SceneMeshBufferAddress(const R_Scene *scene)
{
	return GFX_DeviceBufferAddress(scene->device, scene->mesh_buffer);
}

internal u32
R_SceneResolveTextureBindless(const R_Scene *scene,
							  AST_Assets *assets,
							  AST_Handle handle)
{
	if (!AST_IsValid(assets, handle))
		return 0;

	if (!AST_IsLoaded(assets, handle))
		return 0;

	// TODO: fuck this won't work with hot-reloading will it shiitt.
	
	AST_Asset *texture_asset = AST_GetNow(assets, handle, AST_Type_Texture);

	GFX_TextureKey key = AST_AssetTextureGet(texture_asset);

	GFX_TextureView *view = GFX_DeviceTextureViewFromKey(scene->device, GFX_DeviceTextureViewAuto(scene->device, key));

	return view->bindless.value;
}

internal R_SceneMaterialHandle
R_SceneRegisterMaterial(R_Scene *scene,
						const AST_ModelMaterial *material,
						AST_Assets *assets)
{
	AssertTrue(scene->material_count < R_SCENE_MAX_MATERIALS);

	u32 index = scene->material_count;

	R_GPU_Material *gpu = &scene->gpu_materials[index];

	gpu->albedo_texture                       = R_SceneResolveTextureBindless(scene, assets, material->albedo_texture);
	gpu->normal_texture                       = R_SceneResolveTextureBindless(scene, assets, material->normal_texture);
	gpu->metallic_roughness_texture           = R_SceneResolveTextureBindless(scene, assets, material->metallic_roughness_texture);
	gpu->emissive_texture                     = R_SceneResolveTextureBindless(scene, assets, material->emissive_texture);
	gpu->occlusion_texture                    = R_SceneResolveTextureBindless(scene, assets, material->occlusion_texture);
	
	gpu->albedo_factor                        = material->albedo_factor;
	gpu->normal_scale                         = material->normal_scale;
	gpu->metallic_factor                      = material->metallic_factor;
	gpu->roughness_factor                     = material->roughness_factor;
	gpu->emissive_factor                      = material->emissive_factor;
	gpu->emissive_intensity                   = material->emissive_intensity;
	gpu->occlusion_intensity                  = material->occlusion_intensity;

	// ---
	
	gpu->ior                                  = material->ior;

	// ---
	
	gpu->transmission_texture                 = R_SceneResolveTextureBindless(scene, assets, material->transmission_texture);
	gpu->thickness_texture                    = R_SceneResolveTextureBindless(scene, assets, material->thickness_texture);
	gpu->transmission_factor                  = material->transmission_factor;
	gpu->thickness_factor                     = material->thickness_factor;
	
	gpu->attenuation_colour                   = material->attenuation_colour;
	gpu->attenuation_distance                 = material->attenuation_distance;

	// ---

	gpu->specular_texture                     = R_SceneResolveTextureBindless(scene, assets, material->specular_texture);
	gpu->specular_colour_texture              = R_SceneResolveTextureBindless(scene, assets, material->specular_colour_texture);

	gpu->specular_factor                      = material->specular_factor;
	gpu->specular_colour_factor               = material->specular_colour_factor;

	// ---

	gpu->clearcoat_texture                    = R_SceneResolveTextureBindless(scene, assets, material->clearcoat_texture);
	gpu->clearcoat_roughness_texture          = R_SceneResolveTextureBindless(scene, assets, material->clearcoat_roughness_texture);

	gpu->clearcoat_factor                     = material->clearcoat_factor;
	gpu->clearcoat_roughness_factor           = material->clearcoat_roughness_factor;

	// ---

	gpu->sheen_colour_texture                 = R_SceneResolveTextureBindless(scene, assets, material->sheen_colour_texture);
	gpu->sheen_roughness_texture              = R_SceneResolveTextureBindless(scene, assets, material->sheen_roughness_texture);

	gpu->sheen_colour_factor                  = material->sheen_colour_factor;
	gpu->sheen_roughness_factor               = material->sheen_roughness_factor;

	// ---

	gpu->iridescence_texture                  = R_SceneResolveTextureBindless(scene, assets, material->iridescence_texture);
	gpu->iridescence_thickness_texture        = R_SceneResolveTextureBindless(scene, assets, material->iridescence_thickness_texture);

	gpu->iridescence_factor                   = material->iridescence_factor;
	gpu->iridescence_ior                      = material->iridescence_ior;
	gpu->iridescence_thickness_min_nanometers = material->iridescence_thickness_min_nanometers;
	gpu->iridescence_thickness_max_nanometers = material->iridescence_thickness_max_nanometers;

	// ---

	gpu->double_sided                         = (u32)material->double_sided;
	gpu->unlit                                = (u32)material->unlit;
	gpu->alpha_cutoff                         = material->alpha_cutoff;
	gpu->alpha_mode                           = (u32)material->alpha_mode;
	
	scene->material_count++;
	scene->material_buffer_dirty = true;

	R_SceneMaterialHandle handle = {0};
	handle.value = index;

	DebugLogD(scene->log_channel, "Registered Material.");
	
	return handle;
}

internal u64
R_SceneMaterialBufferAddress(const R_Scene *scene)
{
	return GFX_DeviceBufferAddress(scene->device, scene->material_buffer);
}

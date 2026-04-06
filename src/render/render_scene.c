
internal void
R_SceneInit(R_Scene *scene, Arena *arena, GFX_Device *device)
{
	MemZeroStruct(scene);

	scene->arena = arena;
	scene->device = device;

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
	{
		GFX_Buffer *buf = GFX_DeviceBufferFromKey(scene->device, page->vertex_buffer);
		mapped[i].vertex_buffer = GFX_BufferAddress(buf);
	}
}

internal void
R_SceneUpdateMeshBuffer(R_Scene *scene)
{
	AssertTrue(scene->mesh_count <= R_SCENE_MAX_MESHES);
 
	GFX_Buffer *buf = GFX_DeviceBufferFromKey(scene->device, scene->mesh_buffer);
	GFX_BufferWrite(buf, scene->gpu_meshes, sizeof(R_GPU_RenderMesh) * scene->mesh_count, 0);
}

internal void
R_SceneUpdateMaterialBuffer(R_Scene *scene)
{
	AssertTrue(scene->material_count <= R_SCENE_MAX_MATERIALS);
 
	GFX_Buffer *buf = GFX_DeviceBufferFromKey(scene->device, scene->material_buffer);
	GFX_BufferWrite(buf, scene->gpu_materials, sizeof(R_GPU_Material) * scene->material_count, 0);
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

		b32 fits = ((last->vertex_offset + vertex_count) <= last->max_vertices &&
					(last->index_offset  + index_count)  <= last->max_indices);

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
	vb_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	vb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vb_info.size  = R_PAGE_VERTEX_BUFFER_SIZE;
 
	GFX_BufferAllocInfo ib_info = {0};
	ib_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT;
	ib_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ib_info.size  = R_PAGE_INDEX_BUFFER_SIZE;
 
	R_GeometryPage page = {0};
	page.vertex_buffer = GFX_DeviceBufferAlloc(scene->device, &vb_info);
	page.index_buffer  = GFX_DeviceBufferAlloc(scene->device, &ib_info);
	page.vertex_offset = 0;
	page.index_offset  = 0;
	page.max_vertices  = R_PAGE_VERTEX_BUFFER_SIZE / sizeof(R_GPU_ModelVertex);
	page.max_indices   = R_PAGE_INDEX_BUFFER_SIZE  / sizeof(R_MeshIndex);
	page.next = NULL;
 
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

internal R_SceneMeshHandle
R_SceneRegisterMesh(R_Scene *scene, const R_Mesh *mesh)
{
	AssertTrue(scene->mesh_count < R_SCENE_MAX_MESHES);

	u32 page_index = R_SceneFindSuitablePage(scene, mesh->vertex_count, mesh->index_count);

	R_GeometryPage *page = scene->geometry_page_head;
	for (u32 i = 0; i < page_index; i++, page = page->next); // Iterate to page_index of the page linked list.

	GFX_Buffer *page_vertex_buffer = GFX_DeviceBufferFromKey(scene->device, page->vertex_buffer);
	GFX_Buffer *page_index_buffer  = GFX_DeviceBufferFromKey(scene->device, page->index_buffer);

	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(scene->device);
	{
		const u64 vertex_stride = sizeof(R_GPU_ModelVertex);
		const u64 index_stride	= sizeof(R_MeshIndex);

		VkBufferCopy2 vc = {0};
		vc.sType	 = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
		vc.srcOffset = 0;
		vc.dstOffset = page->vertex_offset * vertex_stride;
		vc.size		 = mesh->vertex_count  * vertex_stride;
		
		VkBufferCopy2 ic = {0};
		ic.sType	 = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
		ic.srcOffset = 0;
		ic.dstOffset = page->index_offset * index_stride;
		ic.size		 = mesh->index_count  * index_stride;
		
		GFX_Buffer *mesh_vertex_buffer = GFX_DeviceBufferFromKey(scene->device, mesh->vertex_buffer);
		GFX_Buffer *mesh_index_buffer  = GFX_DeviceBufferFromKey(scene->device, mesh->index_buffer);

		GFX_CmdCopyBufferToBuffer(&cmd, mesh_vertex_buffer, page_vertex_buffer, 1, &vc);
		GFX_CmdCopyBufferToBuffer(&cmd, mesh_index_buffer,  page_index_buffer,  1, &ic);
	}
	GFX_DeviceSubmitImEnd(scene->device, &cmd);

	u32 mesh_data_index = scene->mesh_count;

	R_GPU_RenderMesh *gpu_mesh = &scene->gpu_meshes[mesh_data_index];

	gpu_mesh->index_count = mesh->index_count;
	gpu_mesh->first_index = page->index_offset;
	gpu_mesh->vertex_buffer = GFX_BufferAddress(page_vertex_buffer) + (page->vertex_offset * sizeof(R_GPU_ModelVertex));
 
	// Advance page allocator.
	page->vertex_offset += mesh->vertex_count;
	page->index_offset	+= mesh->index_count;
 
	// Register the mesh memory location so that the scene object
	// can look it up by handle instead.
	scene->mesh_registry[mesh_data_index].page	= page_index;
	scene->mesh_registry[mesh_data_index].index = mesh_data_index;
 
	scene->mesh_count++;
	scene->mesh_buffer_dirty = true;
 
	R_SceneMeshHandle handle = {0};
	handle.value = mesh_data_index;
	
	return handle;
}

internal R_SceneMaterialHandle
R_SceneRegisterMaterial(R_Scene *scene, const R_Material *material)
{
	AssertTrue(scene->material_count < R_SCENE_MAX_MATERIALS);

	u32 index = scene->material_count;
	
	R_GPU_Material *gpu_material = &scene->gpu_materials[index];

	gpu_material->albedo_texture = 0;
	gpu_material->normal_texture = 0;
	gpu_material->emissive_texture = 0;
	gpu_material->metallic_roughness_texture = 0;
	gpu_material->ambient_texture = 0;

	scene->material_count++;
	scene->material_buffer_dirty = true;

	R_SceneMaterialHandle handle = {0};
	handle.value = index;

	return handle;
}

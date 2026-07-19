
static void R_SceneInit(R_Scene *scene, Arena *arena, LOG_Channel log_channel)
{
	scene->log_channel = log_channel;

	DensePoolInit(&scene->object_pool, arena, R_SCENE_MAX_INSTANCES);
	DensePoolInit(&scene->light_pool, arena, R_SCENE_MAX_LIGHTS);
	SlotPoolInit(&scene->mesh_pool, arena, R_SCENE_MAX_MESHES);
	SlotPoolInit(&scene->material_pool, arena, R_SCENE_MAX_MATERIALS);

	G_BufferAllocInfo mesh_buffer_alloc_info = {0};
	mesh_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	mesh_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	mesh_buffer_alloc_info.size = sizeof(R_GPU_RenderMesh) * R_SCENE_MAX_MESHES;

	scene->mesh_buffer = G_DeviceBufferAlloc(&mesh_buffer_alloc_info);
	scene->mesh_buffer_dirty = true;
	
	G_BufferAllocInfo material_buffer_alloc_info = {0};
	material_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	material_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	material_buffer_alloc_info.size = sizeof(R_GPU_Material) * R_SCENE_MAX_MATERIALS;
	
	scene->material_buffer = G_DeviceBufferAlloc(&material_buffer_alloc_info);
	scene->material_buffer_dirty = true;
}

static void R_SceneDestroy(R_Scene *scene)
{
	G_DeviceBufferDestroy(scene->material_buffer);
	
	G_DeviceBufferDestroy(scene->mesh_buffer);

	for (u32 i = 0; i < scene->geometry_page_count; i++)
	{
		G_DeviceBufferDestroy(scene->geometry_pages[i].vertex_buffer);
		G_DeviceBufferDestroy(scene->geometry_pages[i].index_buffer);
	}
}

static R_InstanceHandle R_SceneInstanceCreate(R_Scene *scene)
{
	R_InstanceHandle handle = {0};
	handle.id = DensePoolGetStableID(&scene->object_pool);

	scene->object_occupied[DensePoolDenseIndex(&scene->object_pool, handle.id)] = true;
	
	return handle;
}

static void R_SceneInstanceDestroy(R_Scene *scene, R_InstanceHandle handle)
{
	scene->object_occupied[DensePoolDenseIndex(&scene->object_pool, handle.id)] = false;
	DensePoolFreeID(&scene->object_pool, handle.id);
}

static void R_SceneSetInstanceTransform(R_Scene *scene, R_InstanceHandle handle, m4 transform)
{
	u32 index = DensePoolDenseIndex(&scene->object_pool, handle.id);
	
	scene->objects[index].transform = transform;
	scene->objects[index].normal_matrix = M4RemoveTranslation(M4Inverse(M4Transpose(transform)));
}

static void R_SceneSetInstanceLocalSphereBounds(R_Scene *scene, R_InstanceHandle handle, v4 local_sphere_bounds)
{
	u32 index = DensePoolDenseIndex(&scene->object_pool, handle.id);
	
	scene->objects[index].local_sphere_bounds = local_sphere_bounds;
}

static void R_SceneSetInstanceMesh(R_Scene *scene, R_InstanceHandle handle, R_MeshHandle mesh)
{
	u32 index = DensePoolDenseIndex(&scene->object_pool, handle.id);
	
	scene->objects[index].page_index = mesh.page_index;
	scene->objects[index].mesh_index = mesh.slot_index;
}

static void R_SceneSetInstanceMaterial(R_Scene *scene, R_InstanceHandle handle, R_MaterialHandle material)
{
	u32 index = DensePoolDenseIndex(&scene->object_pool, handle.id);
	
	scene->objects[index].material_index = material.index;
}

static void R_SceneSetInstanceSkinning(R_Scene *scene, R_InstanceHandle handle, const m4 *palette, u32 joint_count)
{
	u32 index = DensePoolDenseIndex(&scene->object_pool, handle.id);
	
	scene->objects[index].skinning_palette = palette;
	scene->objects[index].skinning_joint_count = joint_count;
}

static R_LightHandle R_SceneLightCreate(R_Scene *scene)
{
	R_LightHandle handle = {0};
	handle.id = DensePoolGetStableID(&scene->light_pool);

	scene->light_occupied[DensePoolDenseIndex(&scene->light_pool, handle.id)] = true;
	
	return handle;
}

static void R_SceneLightDestroy(R_Scene *scene, R_LightHandle handle)
{
	scene->light_occupied[DensePoolDenseIndex(&scene->light_pool, handle.id)] = false;
	DensePoolFreeID(&scene->light_pool, handle.id);
}

static void R_SceneSetLight(R_Scene *scene, R_LightHandle handle, R_Light light)
{
	u32 index = DensePoolDenseIndex(&scene->light_pool, handle.id);
	
	scene->lights[index] = light;
}

static R_MeshHandle R_SceneAllocMesh(R_Scene *scene, const R_MeshDesc *desc)
{
	u32 slot_index = 0;

	if (!SlotPoolAlloc(&scene->mesh_pool, &slot_index))
	{
		DebugLogE(scene->log_channel, "Unable to allocate more meshes!");
		return R_MeshHandleNull();
	}

	u32 page_index = R_SceneFindSuitablePage(scene, desc->vertex_count, desc->index_count);
	R_GeometryPage *page = &scene->geometry_pages[page_index];

	u64 vertex_offset = 0;
	u64 index_offset = 0;
	
	b32 vok = R_GeometryFreeListTryAlloc(&page->vertex_free, desc->vertex_count, &vertex_offset);
	b32 iok = R_GeometryFreeListTryAlloc(&page->index_free,  desc->index_count,  &index_offset);

	DebugLogAssert(scene->log_channel, vok, "Vertex region allocation failed after R_GeometryFreeListAvailable returned true.");
	DebugLogAssert(scene->log_channel, iok, "Index region allocation failed after R_GeometryFreeListAvailable returned true.");
	
	const u64 vertex_stride = sizeof(R_GPU_ModelVertex);
	const u64 index_stride = sizeof(A_ModelIndex);

	G_BufferCopy vc = {0};
	vc.src_offset = 0;
	vc.dst_offset = vertex_offset * vertex_stride;
	vc.size = desc->vertex_count * vertex_stride;

	G_BufferCopy ic = {0};
	ic.src_offset = 0;
	ic.dst_offset = index_offset * index_stride;
	ic.size = desc->index_count * index_stride;

	R_ScenePageMeshCopy copy = {0};
	copy.vertices = desc->vertices;
	copy.vertex_size = desc->vertex_count * vertex_stride;
	copy.vertex_offset_dst = vertex_offset * vertex_stride;
	copy.indices = desc->indices;
	copy.index_size = desc->index_count * index_stride;
	copy.index_offset_dst = index_offset * index_stride;
	copy.dst_page_index = page_index;

	DebugLogAssert(scene->log_channel,
				   scene->page_mesh_copy_count < ArraySize(scene->page_mesh_copies),
				   "Ran out of mesh upload buffer!!!!!!!!!!! SHITTTTTT");

	scene->page_mesh_copies[scene->page_mesh_copy_count++] = copy;
	
	R_GPU_RenderMesh *gpu_mesh = &scene->gpu_meshes[slot_index];
	gpu_mesh->index_count = desc->index_count;
	gpu_mesh->first_index = index_offset;
	gpu_mesh->vertex_buffer = G_DeviceBufferAddress(page->vertex_buffer) + (vertex_offset * sizeof(R_GPU_ModelVertex));

	if (!G_BufferKeyIsNull(desc->skin_buffer))
		gpu_mesh->skin_buffer = G_DeviceBufferAddress(desc->skin_buffer);
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
	
	scene->mesh_buffer_dirty = true;
	
	R_MeshHandle handle = {0};
	handle.slot_index = slot_index;
	handle.page_index = page_index;

	return handle;
}

static void R_SceneFreeMesh(R_Scene *scene, R_MeshHandle handle)
{
	SlotPoolFree(&scene->mesh_pool, handle.slot_index);
}

static u32 R_SceneCountOfMeshes(const R_Scene *scene)
{
	return SlotPoolLiveCount(&scene->mesh_pool);
}

static u32 R_SceneFindSuitablePage(R_Scene *scene, u32 vertex_count, u32 index_count)
{
	for (u32 i = 0; i < scene->geometry_page_count; i++)
	{
		R_GeometryPage *page = &scene->geometry_pages[i];
		
		if (R_GeometryFreeListHasAvailable(&page->vertex_free, vertex_count) &&
			R_GeometryFreeListHasAvailable(&page->index_free, index_count))
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

static R_GeometryPage R_SceneCreateNewPage(R_Scene *scene)
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
	page.vertex_buffer = G_DeviceBufferAlloc(&vb_info);
	page.index_buffer = G_DeviceBufferAlloc(&ib_info);
	page.vertex_count = 0;
	page.index_count = 0;
	page.max_vertices = max_vertices;
	page.max_indices = max_indices;

	R_GeometryFreeListInit(&page.vertex_free, max_vertices);
	R_GeometryFreeListInit(&page.index_free, max_indices);
 
	return page;
}

static u32 R_ScenePageCount(const R_Scene *scene)
{
	return scene->geometry_page_count;
}

static R_MaterialHandle R_SceneAddMaterial(R_Scene *scene, const R_Material *material)
{
	u32 slot_index = 0;

	if (!SlotPoolAlloc(&scene->material_pool, &slot_index))
	{
		DebugLogE(scene->log_channel, "Unable to allocate more materials!");
		return R_MaterialHandleNull();
	}

	scene->cpu_materials[slot_index] = *material;
	
	R_SceneBakeMaterialIntoGPU(scene, material, &scene->gpu_materials[slot_index]);
	scene->material_buffer_dirty = true;

	R_MaterialHandle handle = {0};
	handle.index = slot_index;

	return handle;
}

static R_MaterialHandle R_SceneAddMaterialFromAssets(R_Scene *scene, const A_ModelMaterial *source)
{
	R_Material render_material = R_MaterialFromAsset(source);
	return R_SceneAddMaterial(scene, &render_material);
}

static void R_SceneUpdateMaterial(R_Scene *scene, R_MaterialHandle handle, const R_Material *material)
{
	scene->cpu_materials[handle.index] = *material;
	scene->material_buffer_dirty = true;
}

static void R_SceneFreeMaterial(R_Scene *scene, R_MaterialHandle handle)
{
	SlotPoolFree(&scene->material_pool, handle.index);
}

static u32 R_SceneCountOfMaterials(const R_Scene *scene)
{
	return SlotPoolLiveCount(&scene->material_pool);
}

static const R_Material *R_SceneGetMaterial(const R_Scene *scene, R_MaterialHandle handle)
{
	return &scene->cpu_materials[handle.index];
}

static void R_SceneFlushIfDirty(R_Scene *scene)
{
	if (scene->mesh_buffer_dirty)
	{
		if (scene->page_mesh_copy_count > 0)
		{
			G_CmdBuffer cmd = G_DeviceSubmitImBegin();

			for (u32 i = 0; i < scene->page_mesh_copy_count; i++)
			{
				R_ScenePageMeshCopy *copy = &scene->page_mesh_copies[i];
				
				R_GeometryPage *page = &scene->geometry_pages[copy->dst_page_index];
				
				uptr vertices_mapped = (uptr)G_DeviceBufferMap(page->vertex_buffer);
				uptr indices_mapped = (uptr)G_DeviceBufferMap(page->index_buffer);

				MemCopy((void *)(vertices_mapped + copy->vertex_offset_dst), copy->vertices, copy->vertex_size);
				MemCopy((void *)(indices_mapped + copy->index_offset_dst), copy->indices, copy->index_size);
			}
	
			G_DeviceSubmitImEnd(&cmd);
		}
		
		G_DeviceBufferWrite(scene->mesh_buffer, scene->gpu_meshes, sizeof(scene->gpu_meshes), 0);
		scene->mesh_buffer_dirty = false;
	}

	if (scene->material_buffer_dirty)
	{
		G_DeviceBufferWrite(scene->material_buffer, scene->gpu_materials, sizeof(scene->gpu_materials), 0);
		scene->material_buffer_dirty = false;
	}
}

static void R_SceneBakeMaterialIntoGPU(const R_Scene *scene, const R_Material *material, R_GPU_Material *out)
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

static u32 R_SceneResolveToBindlessIndex(const R_Scene *scene, G_TextureKey key)
{
	if (G_TextureKeyIsNull(key))
		return G_BINDLESS_INDEX_INVALID;

	G_TextureViewKey view_key = G_DeviceTextureViewAuto(key);
	return G_DeviceTextureViewBindless(view_key);
}


internal void R_FrameParamsUploadPageTable(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->page_count = R_ScenePageCount(scene);

	u32 alloc_count = out->page_count > 0 ? out->page_count : 1;
	out->page_table_buffer = G_RingBufferPushArray(ring, R_GPU_PagePointers, alloc_count);

	R_GPU_PagePointers *mapped = out->page_table_buffer.cpu;

	for (u32 i = 0; i < out->page_count; i++)
	{
		mapped[i].vertex_buffer = G_DeviceBufferAddress(scene->geometry_pages[i].vertex_buffer);
		out->page_index_buffers[i] = scene->geometry_pages[i].index_buffer;
	}
}

internal void R_FrameParamsUploadSkinning(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	u32 total_joints = 0;

	R_EntityIterator iterator = R_EntityIteratorInit(scene);
	R_Entity *entity = NULL;

	while ((entity = R_EntityIteratorNext(&iterator, R_EntityType_Object)))
		total_joints += entity->object.skinning_joint_count;

	if (total_joints == 0)
		return;

	out->skinning_palette_buffer = G_RingBufferPushArray(ring, m4, total_joints);
	m4 *mapped = out->skinning_palette_buffer.cpu;
	u32 offset = 0;

	u32 object_index = 0;

	R_EntityIteratorReset(&iterator);
	
	while ((entity = R_EntityIteratorNext(&iterator, R_EntityType_Object)))
	{
		R_Object *object = &entity->object;
		
		u32 joints = object->skinning_joint_count;
		
		if (joints > 0)
		{
			MemCopy(mapped + offset, object->skinning_palette, joints * sizeof(m4));
		
			object->skinning_address = out->skinning_palette_buffer.gpu + offset * sizeof(m4);

			offset += joints;	
		}

		object_index++;
	}
}

internal void R_FrameParamsUploadObjects(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->object_buffer = G_RingBufferPushArray(ring, R_GPU_ObjectData, out->object_count);
	
	R_GPU_ObjectData *mapped = out->object_buffer.cpu;

	u32 object_index = 0;
	
	R_EntityIterator iterator = R_EntityIteratorInit(scene);
	R_Entity *entity = NULL;
	
	while ((entity = R_EntityIteratorNext(&iterator, R_EntityType_Object)))
	{
		R_Object *object = &entity->object;
		
		mapped[object_index].model_matrix = object->transform;
		mapped[object_index].normal_matrix = object->normal_matrix;
		mapped[object_index].local_sphere_bounds = object->local_sphere_bounds;
		mapped[object_index].material_index = object->material_index;
		mapped[object_index].mesh_index = object->mesh_index;
		mapped[object_index].page_index = object->page_index;
		mapped[object_index].skinning_palette_buffer = object->skinning_address;
		mapped[object_index].skinning_joint_count = object->skinning_joint_count;

		object_index++;
	}
}

internal void R_FrameParamsUploadLights(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->light_buffer = G_RingBufferPushArray(ring, R_GPU_Light, out->light_count);
	
	R_GPU_Light *mapped = out->light_buffer.cpu;

	u32 light_index = 0;

	R_EntityIterator iterator = R_EntityIteratorInit(scene);
	R_Entity *entity = NULL;
	
	while ((entity = R_EntityIteratorNext(&iterator, R_EntityType_Light)))
	{
		R_Light *light = &entity->light;
		
		const f32 radius = R_LightHeuristicRadius(light, 0.05f);

		mapped[light_index].transform = M4Transform(light->position, V4QuatIdentity(), v3x(radius), v3x(0.f));
		mapped[light_index].position = light->position;
		mapped[light_index].colour = light->colour;
		mapped[light_index].intensity = light->intensity;
		mapped[light_index].attenuation = v3(light->falloff, 0.f, 0.f);
		mapped[light_index].radius = radius;

		if (light->casts_shadows && out->shadow_caster_count < ArraySize(out->shadow_casters))
		{
			mapped[light_index].shadow_slot_index = (i32)out->shadow_caster_count;

			R_ShadowCaster *caster = &out->shadow_casters[out->shadow_caster_count++];
			caster->position = light->position;
			caster->near = light->shadow_near;
			caster->far = light->shadow_far;
			caster->radius = radius;
		}
		else
		{
			mapped[light_index].shadow_slot_index = -1;
		}

		light_index++;
	}
}

internal void R_FrameParamsUploadFrameData(G_RingBuffer *ring, R_FrameParams *out)
{
	u32 window_width, window_height;
	osapi->GetWindowSize(&window_width, &window_height);

	const R_Camera *camera = &out->camera;

	out->frame_data = G_RingBufferPushArray(ring, R_GPU_FrameData, 1);

	R_GPU_FrameData *frame_data = out->frame_data.cpu;

	frame_data->view = camera->view;
	frame_data->proj = camera->proj;
	frame_data->view_proj = camera->view_proj;
	frame_data->view_proj_no_translation = camera->view_proj_no_translation;
	frame_data->inv_view = camera->inv_view;
	frame_data->inv_proj = camera->inv_proj;
	frame_data->camera_position = camera->position;
	frame_data->window_resolution = v2(window_width, window_height);
	frame_data->frame_number = out->frame_number;
	frame_data->delta_time = out->dt;
	frame_data->time = out->elapsed;
}

internal void R_FrameParamsResolveShaders(R_System *system, R_FrameParams *out)
{
	out->debug_line_shader             = A_GetOrBreak(system->shaders.debug_line_handle)->shader.key;
	out->forward_shader                = A_GetOrBreak(system->shaders.forward_handle)->shader.key;
	out->shadow_shader                 = A_GetOrBreak(system->shaders.shadow_handle)->shader.key;
	out->cull_frustum_shader           = A_GetOrBreak(system->shaders.cull_frustum_handle)->shader.key;
	out->cull_sphere_shader            = A_GetOrBreak(system->shaders.cull_sphere_handle)->shader.key;
	out->skybox_shader                 = A_GetOrBreak(system->shaders.skybox_handle)->shader.key;
	out->tonemapping_shader            = A_GetOrBreak(system->shaders.tonemapping_handle)->shader.key;
	out->brdf_lut_generation_shader    = A_GetOrBreak(system->shaders.brdf_lut_generation_handle)->shader.key;
	out->hdr_to_cubemap_shader         = A_GetOrBreak(system->shaders.hdr_to_cubemap_handle)->shader.key;
	out->irradiance_cubemap_gen_shader = A_GetOrBreak(system->shaders.irradiance_cubemap_gen_handle)->shader.key;
	out->prefilter_cubemap_gen_shader  = A_GetOrBreak(system->shaders.prefilter_cubemap_gen_handle)->shader.key;
}

internal R_FrameParams R_FrameParamsBuild(Arena *frame_arena,
										R_System *system,
										u32 frame_number, f32 dt, f32 elapsed,
										R_Scene *scene, const R_Camera *camera)
{
	G_RingBuffer *ring = &system->frame_upload_ring_buffer;

	R_FrameParams params = {0};

	params.arena = frame_arena;
	params.frame_number = frame_number;
	params.dt = dt;
	params.elapsed = elapsed;
	params.camera = *camera;

	params.mesh_buffer = scene->mesh_buffer;
	params.material_buffer = scene->material_buffer;

	params.brdf_lut = system->brdf_lut;
	params.irradiance_fallback_cubemap = system->irradiance_cubemap;
	params.prefilter_cubemap = system->prefilter_cubemap;

	params.skybox_mesh = &system->skybox_mesh;
	
	params.cubemap_capture_transform_buffer = system->cubemap_capture_transform_buffer;

	params.linear_sampler = system->samplers.linear;
	params.nearest_sampler = system->samplers.nearest;

	params.object_count = scene->entity_count[R_EntityType_Object];
	params.light_count = scene->entity_count[R_EntityType_Light];
	params.shadow_caster_count = 0;
	
	R_FrameParamsUploadPageTable(scene, ring, &params);

	if (params.object_count > 0)
	{
		R_FrameParamsUploadSkinning(scene, ring, &params);
		R_FrameParamsUploadObjects(scene, ring, &params);
	}

	if (params.light_count > 0)
	{
		R_FrameParamsUploadLights(scene, ring, &params);
	}

	R_FrameParamsUploadFrameData(ring, &params);
	R_FrameParamsResolveShaders(system, &params);

	return params;
}

internal void R_FrameParamsDrawIndirect(const R_FrameParams *frame_params,
									  G_CmdBuffer *cmd,
									  G_ResourceKey indirect_buffer,
									  G_ResourceKey count_buffer)
{
	const u64 max_draws_per_page = R_SCENE_MAX_ENTITIES;

	for (u32 i = 0; i < frame_params->page_count; i++)
	{
		u64 indirect_offset = i * sizeof(R_GPU_IndirectDraw) * max_draws_per_page;
		u64 count_offset = i * sizeof(u32);

		G_CmdBindIndexBuffer(cmd,
							 frame_params->page_index_buffers[i],
							 0, VK_WHOLE_SIZE,
							 VK_INDEX_TYPE_UINT32);

		G_CmdDrawIndexedIndirectCount(cmd,
									  indirect_buffer, indirect_offset,
									  count_buffer, count_offset,
									  max_draws_per_page,
									  sizeof(R_GPU_IndirectDraw));
	}
}


static void R_FrameParamsUploadPageTable(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->page_count = R_ScenePageCount(scene); // real count, may be 0

	u32 alloc_count = out->page_count > 0 ? out->page_count : 1;
	out->page_table_buffer = G_RingBufferPushArray(ring, R_GPU_PagePointers, alloc_count);

	R_GPU_PagePointers *mapped = out->page_table_buffer.cpu;

	for (u32 i = 0; i < out->page_count; i++)
	{
		mapped[i].vertex_buffer = G_DeviceBufferAddress(scene->geometry_pages[i].vertex_buffer);
		out->page_index_buffers[i] = scene->geometry_pages[i].index_buffer;
	}
}

static void R_FrameParamsUploadSkinning(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	u32 total_joints = 0;
	u32 object_count = DensePoolLiveCount(&scene->object_pool);

	for (u32 i = 0; i < object_count; i++)
		total_joints += scene->skinning_joint_counts[i];

	if (total_joints == 0)
		return;

	out->skinning_palette_buffer = G_RingBufferPushArray(ring, m4, total_joints);
	m4 *mapped = out->skinning_palette_buffer.cpu;
	u32 offset = 0;

	for (u32 i = 0; i < object_count; i++)
	{
		u32 joints = scene->skinning_joint_counts[i];
		
		if (joints == 0)
			continue;

		MemCopy(mapped + offset, scene->skinning_palettes[i], joints * sizeof(m4));
		scene->skinning_addrs[i] = out->skinning_palette_buffer.gpu + offset * sizeof(m4);
		offset += joints;
	}
}

static void R_FrameParamsUploadObjects(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->object_count = DensePoolLiveCount(&scene->object_pool);

	if (out->object_count == 0)
		return;

	out->object_buffer = G_RingBufferPushArray(ring, R_GPU_ObjectData, out->object_count);
	R_GPU_ObjectData *mapped = out->object_buffer.cpu;

	for (u32 i = 0; i < out->object_count; i++)
	{
		mapped[i].model_matrix = scene->transforms[i];
		mapped[i].normal_matrix = scene->normal_matrices[i];
		mapped[i].sphere_bounds = scene->sphere_bounds[i];
		mapped[i].material_index = scene->material_indices[i];
		mapped[i].mesh_index = scene->mesh_indices[i];
		mapped[i].page_index = scene->page_indices[i];
		mapped[i].skinning_palette_buffer = scene->skinning_addrs[i];
		mapped[i].skinning_joint_count = scene->skinning_joint_counts[i];
	}
}

static void R_FrameParamsUploadLights(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->light_count = scene->light_pool.count;
	out->shadow_caster_count = 0;

	if (out->light_count == 0)
		return;

	out->light_buffer = G_RingBufferPushArray(ring, R_GPU_Light, out->light_count);
	R_GPU_Light *mapped = out->light_buffer.cpu;

	for (u32 i = 0; i < out->light_count; i++)
	{
		const R_Light *light = &scene->lights[i];
		const f32 radius = R_LightHeuristicRadius(light, 0.05f);

		mapped[i].transform = M4Transform(light->position, V4QuatIdentity(), v3x(radius), v3x(0.f));
		mapped[i].position = light->position;
		mapped[i].colour = light->colour;
		mapped[i].intensity = light->intensity;
		mapped[i].attenuation = v3(light->falloff, 0.f, 0.f);
		mapped[i].radius = radius;

		if (light->casts_shadows && out->shadow_caster_count < ArraySize(out->shadow_casters))
		{
			mapped[i].shadow_slot_index = (i32)out->shadow_caster_count;

			R_ShadowCaster *caster = &out->shadow_casters[out->shadow_caster_count++];
			caster->position = light->position;
			caster->near = light->shadow_near;
			caster->far  = light->shadow_far;
			caster->radius = radius;
		}
		else
		{
			mapped[i].shadow_slot_index = -1;
		}
	}
}

static void R_FrameParamsUploadFrameData(G_RingBuffer *ring, R_FrameParams *out)
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

static R_FrameParams R_FrameParamsBuild(Arena *frame_arena,
										R_System *system,
										f32 dt, f32 elapsed,
										R_Scene *scene, const R_Camera *camera)
{
	G_RingBuffer *ring = &system->frame_upload_ring_buffer;

	R_FrameParams params = {0};

	params.arena = frame_arena;
	params.frame_number = system->frame_count++;
	params.dt = dt;
	params.elapsed = elapsed;
	params.camera = *camera;

	params.linear_sampler = system->linear_sampler;
	params.nearest_sampler = system->nearest_sampler;
	params.irradiance_fallback_cubemap = system->irradiance_cubemap;
	params.prefilter_cubemap = system->prefilter_cubemap;
	params.brdf = system->brdf_lut;

	params.mesh_buffer = scene->mesh_buffer;
	params.material_buffer = scene->material_buffer;

	R_FrameParamsUploadPageTable(scene, ring, &params);
	R_FrameParamsUploadSkinning(scene, ring, &params);
	R_FrameParamsUploadObjects(scene, ring, &params);
	R_FrameParamsUploadLights(scene, ring, &params);
	R_FrameParamsUploadFrameData(ring, &params);

	return params;
}

static void R_FrameParamsDrawIndirect(const R_FrameParams *frame_params,
									  G_CmdBuffer *cmd,
									  G_BufferKey indirect_buffer,
									  G_BufferKey count_buffer)
{
	const u64 max_draws_per_page = R_SCENE_MAX_INSTANCES;

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

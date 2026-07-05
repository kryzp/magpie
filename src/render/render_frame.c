
static void R_FrameParamsUploadPageTable(const R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->page_count = scene->meshes.geometry_page_count; // real count, may be 0

	u32 alloc_count = out->page_count > 0 ? out->page_count : 1;
	out->page_table_buffer = G_RingBufferPushArray(ring, R_GPU_PagePointers, alloc_count);

	R_GPU_PagePointers *mapped = out->page_table_buffer.cpu;

	for (u32 i = 0; i < out->page_count; i++)
	{
		mapped[i].vertex_buffer = G_DeviceBufferAddress(scene->meshes.geometry_pages[i].vertex_buffer);
		out->page_index_buffers[i] = scene->meshes.geometry_pages[i].index_buffer;
	}
}

static void R_FrameParamsUploadSkinning(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	u32 total_joints = 0;

	for (u32 i = 0; i < ArraySize(scene->graph.object_slots); i++)
	{
		R_ObjectSlot *slot = &scene->graph.object_slots[i];
		slot->skinning_palette_gpu_addr = 0;

		if (!slot->active || slot->skinning_palette == NULL)
			continue;

		total_joints += slot->skinning_joint_count;
	}

	if (total_joints == 0)
		return;

	out->skinning_palette_buffer = G_RingBufferPushArray(ring, m4, total_joints);

	m4 *mapped = out->skinning_palette_buffer.cpu;
	u32 offset = 0;

	for (u32 i = 0; i < ArraySize(scene->graph.object_slots); i++)
	{
		R_ObjectSlot *slot = &scene->graph.object_slots[i];

		if (!slot->active || slot->skinning_palette == NULL)
			continue;

		u32 joints = slot->skinning_joint_count;
		MemCopy(mapped + offset, slot->skinning_palette, joints * sizeof(m4));
		slot->skinning_palette_gpu_addr = out->skinning_palette_buffer.gpu + offset * sizeof(m4);
		offset += joints;
	}
}

static void R_FrameParamsUploadObjects(const R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->object_count = scene->graph.object_count;
	out->object_buffer = G_RingBufferPushArray(ring, R_GPU_ObjectData, out->object_count);

	R_GPU_ObjectData *mapped = out->object_buffer.cpu;
	u32 write_index = 0;

	for (u32 i = 0; i < ArraySize(scene->graph.object_slots) && write_index < out->object_count; i++)
	{
		const R_ObjectSlot *slot = &scene->graph.object_slots[i];

		if (!slot->active)
			continue;

		u32 mesh_index = slot->mesh.index;
		u32 material_index = slot->material.index;
		u32 page_index = scene->meshes.mesh_slots[mesh_index].page_index;

		mapped[write_index].model_matrix = slot->transform;
		mapped[write_index].normal_matrix = M4RemoveTranslation(M4Inverse(M4Transpose(slot->transform)));
		mapped[write_index].sphere_bounds = slot->sphere_bounds;
		mapped[write_index].material_index = material_index;
		mapped[write_index].mesh_index = mesh_index;
		mapped[write_index].page_index = page_index;
		mapped[write_index].skinning_palette_buffer = slot->skinning_palette_gpu_addr;
		mapped[write_index].skinning_joint_count = slot->skinning_joint_count;

		write_index++;
	}
}

static void R_FrameParamsUploadLights(const R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->light_count = scene->graph.light_count;
	out->shadow_caster_count = 0;

	out->light_buffer = G_RingBufferPushArray(ring, R_GPU_Light, out->light_count);

	R_GPU_Light *mapped = out->light_buffer.cpu;
	u32 write_index = 0;

	for (u32 i = 0; i < ArraySize(scene->graph.light_slots) && write_index < out->light_count; i++)
	{
		const R_LightSlot *slot = &scene->graph.light_slots[i];

		if (!slot->active)
			continue;

		const R_Light *light = &slot->light;
		const f32 heuristic_radius = R_LightHeuristicRadius(light, 0.05f);

		mapped[write_index].transform = M4Transform(light->position, V4QuatIdentity(), v3x(heuristic_radius), v3x(0.f));
		mapped[write_index].position = light->position;
		mapped[write_index].colour = light->colour;
		mapped[write_index].intensity = light->intensity;
		mapped[write_index].attenuation = v3(light->falloff, 0.f, 0.f);
		mapped[write_index].radius = heuristic_radius;

		if (light->casts_shadows && out->shadow_caster_count < ArraySize(out->shadow_casters))
		{
			mapped[write_index].shadow_slot_index = (i32)out->shadow_caster_count;

			R_ShadowCaster *caster = &out->shadow_casters[out->shadow_caster_count];
			caster->position = light->position;
			caster->near = light->shadow_near;
			caster->far = light->shadow_far;
			caster->radius = heuristic_radius;

			out->shadow_caster_count++;
		}
		else
		{
			mapped[write_index].shadow_slot_index = -1;
		}

		write_index++;
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

static R_FrameParams R_FrameParamsBuild(R_System *system, Arena *frame_arena, f32 dt, f32 elapsed, R_Scene *scene, const R_Camera *camera)
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

	params.mesh_buffer = scene->meshes.mesh_buffer;
	params.material_buffer = scene->materials.material_buffer;

	R_FrameParamsUploadPageTable(scene, ring, &params);

	if (scene->graph.object_count > 0)
	{
		R_FrameParamsUploadSkinning(scene, ring, &params);
		R_FrameParamsUploadObjects(scene, ring, &params);
	}

	if (scene->graph.light_count > 0)
	{
		R_FrameParamsUploadLights(scene, ring, &params);
	}

	R_FrameParamsUploadFrameData(ring, &params);

	return params;
}

static void R_FrameParamsDrawIndirect(const R_FrameParams *frame_params,
									  G_CmdBuffer *cmd,
									  G_BufferKey indirect_buffer,
									  G_BufferKey count_buffer)
{
	const u64 max_draws_per_page = R_SCENE_GRAPH_MAX_OBJECTS;

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


static void R_FrameParamsUploadPageTable(const R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out)
{
	out->page_count = scene->meshes.geometry_page_count > 0 ? scene->meshes.geometry_page_count : 1;

	out->page_table_buffer = G_RingBufferPushArray(ring, R_GPU_PagePointers, out->page_count);

	R_GPU_PagePointers *mapped = out->page_table_buffer.cpu;

	for (u32 i = 0; i < scene->meshes.geometry_page_count; i++)
		mapped[i].vertex_buffer = G_DeviceBufferAddress(scene->meshes.device, scene->meshes.geometry_pages[i].vertex_buffer);
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

		mapped[write_index].model_matrix            = slot->transform;
		mapped[write_index].normal_matrix           = M4RemoveTranslation(M4Inverse(M4Transpose(slot->transform)));
		mapped[write_index].sphere_bounds           = slot->sphere_bounds;
		mapped[write_index].material_index          = material_index;
		mapped[write_index].mesh_index              = mesh_index;
		mapped[write_index].page_index              = page_index;
		mapped[write_index].skinning_palette_buffer = slot->skinning_palette_gpu_addr;
		mapped[write_index].skinning_joint_count    = slot->skinning_joint_count;

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

		mapped[write_index].transform   = M4Transform(light->position, V4QuatIdentity(), v3x(heuristic_radius), v3x(0.f));
		mapped[write_index].position    = light->position;
		mapped[write_index].colour      = light->colour;
		mapped[write_index].intensity   = light->intensity;
		mapped[write_index].attenuation = v3(light->falloff, 0.f, 0.f);
		mapped[write_index].radius      = heuristic_radius;

		if (light->casts_shadows && out->shadow_caster_count < ArraySize(out->shadow_casters))
		{
			mapped[write_index].shadow_slot_index = (i32)out->shadow_caster_count;

			R_ShadowCaster *caster = &out->shadow_casters[out->shadow_caster_count];
			caster->position = light->position;
			caster->near     = light->shadow_near;
			caster->far      = light->shadow_far;
			caster->radius   = heuristic_radius;

			out->shadow_caster_count++;
		}
		else
		{
			mapped[write_index].shadow_slot_index = -1;
		}

		write_index++;
	}
}

static R_FrameParams R_FrameParamsBuild(R_System *system, Arena *frame_arena, f32 dt, f32 elapsed, R_Scene *scene, const R_Camera *camera)
{
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

	R_FrameParamsUploadPageTable(scene, &system->frame_upload_ring_buffer, &params);

	if (scene->graph.object_count > 0)
	{
		R_FrameParamsUploadSkinning(scene, &system->frame_upload_ring_buffer, &params);
		R_FrameParamsUploadObjects(scene, &system->frame_upload_ring_buffer, &params);
	}

	if (scene->graph.light_count > 0)
	{
		R_FrameParamsUploadLights(scene, &system->frame_upload_ring_buffer, &params);
	}

	return params;
}

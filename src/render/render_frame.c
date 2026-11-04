
internal void R_FramePacketBegin(R_FramePacket *packet, u64 frame_number, f32 dt, f32 elapsed)
{
	Arena arena = packet->arena;
	ArenaReset(&arena);

	MemZeroStruct(packet);
	packet->arena = arena;

	packet->frame_number = frame_number;
	packet->dt = dt;
	packet->elapsed = elapsed;

	osapi->GetWindowSize(&packet->window_width, &packet->window_height);
}

internal void R_FramePacketCollectScene(R_FramePacket *packet, const R_Scene *scene, const R_Camera *camera, f32 interp_t)
{
	Arena *arena = &packet->arena;

	packet->camera = *camera;
	
	packet->mesh_count = scene->mesh_high_water;
	packet->meshes = ArenaPushArray(arena, R_GPU_RenderMesh, MaxValue(packet->mesh_count, 1));
	MemCopy(packet->meshes, scene->gpu_meshes, packet->mesh_count * sizeof(R_GPU_RenderMesh));

	packet->material_count = scene->material_high_water;
	packet->materials = ArenaPushArray(arena, R_GPU_Material, MaxValue(packet->material_count, 1));
	MemCopy(packet->materials, scene->gpu_materials, packet->material_count * sizeof(R_GPU_Material));

	packet->page_count = scene->geometry_page_count;

	for (u32 i = 0; i < packet->page_count; i++)
	{
		const R_GeometryPage *page = &scene->geometry_pages[i];

		packet->pages[i].vertex_buffer_address = G_BufferAddress(page->vertex_buffer);
		packet->pages[i].index_buffer = page->index_buffer;
		packet->pages[i].index_type = page->index_type;
	}
	
	u32 object_count = 0;
	u32 light_count = 0;
	u32 joint_total = 0;

	for (u32 i = 0; i < DensePoolLiveCount(&scene->entity_pool); i++)
	{
		const R_Entity *entity = &scene->entities[i];

		if (entity->type == R_EntityType_Object)
		{
			object_count++;

			if (entity->object.skinning_palette)
				joint_total += entity->object.skinning_joint_count;
		}
		else if (entity->type == R_EntityType_Light)
		{
			light_count++;
		}
	}

	packet->objects = ArenaPushArray(arena, R_PacketObject, MaxValue(object_count, 1));
	packet->lights = ArenaPushArray(arena, R_Light, MaxValue(light_count, 1));
	packet->skinning_matrices = ArenaPushArray(arena, m4, MaxValue(joint_total, 1));

	for (u32 i = 0; i < DensePoolLiveCount(&scene->entity_pool); i++)
	{
		const R_Entity *entity = &scene->entities[i];

		if (entity->type == R_EntityType_Light)
		{
			packet->lights[packet->light_count++] = entity->light;
			continue;
		}

		if (entity->type != R_EntityType_Object)
			continue;

		const R_Object *object = &entity->object;
		R_PacketObject *out = &packet->objects[packet->object_count++];

		// only rebuild if moved
		if (interp_t < 1.f && !M4Equals(object->prev_transform, object->transform))
		{
			out->transform = M4Lerp(object->prev_transform, object->transform, interp_t);
			out->normal_matrix = M4RemoveTranslation(M4Inverse(M4Transpose(out->transform)));
		}
		else
		{
			out->transform = object->transform;
			out->normal_matrix = object->normal_matrix;
		}
		
		out->local_sphere_bounds = object->local_sphere_bounds;
		out->page_index = object->page_index;
		out->mesh_index = object->mesh_index;
		out->material_index = object->material_index;
		out->flags = 0;

		if (object->material_index < packet->material_count &&
			packet->materials[object->material_index].alpha_mode == A_AlphaMode_Blend)
		{
			out->flags |= R_ObjectFlag_Translucent;
		}

		if (object->skinning_palette && object->skinning_joint_count > 0)
		{
			out->skinning_first_matrix = packet->skinning_matrix_count;
			out->skinning_joint_count = object->skinning_joint_count;

			MemCopy(packet->skinning_matrices + packet->skinning_matrix_count,
					object->skinning_palette,
					object->skinning_joint_count * sizeof(m4));

			packet->skinning_matrix_count += object->skinning_joint_count;
		}
	}
}

internal void R_FrameParamsUploadSceneTables(const R_FramePacket *packet, G_RingBuffer *upload, R_FrameParams *out)
{
	out->mesh_table = G_RingBufferPushCopyArray(upload, R_GPU_RenderMesh, packet->meshes, MaxValue(packet->mesh_count, 1));
	out->material_table = G_RingBufferPushCopyArray(upload, R_GPU_Material, packet->materials, MaxValue(packet->material_count, 1));

	out->page_count = packet->page_count;
	out->page_table_buffer = G_RingBufferPushArray(upload, R_GPU_PagePointers, MaxValue(packet->page_count, 1));

	R_GPU_PagePointers *mapped = out->page_table_buffer.cpu;

	for (u32 i = 0; mapped && i < packet->page_count; i++)
	{
		mapped[i].vertex_buffer = packet->pages[i].vertex_buffer_address;
		out->page_index_buffers[i] = packet->pages[i].index_buffer;
		out->page_index_types[i] = packet->pages[i].index_type;
	}
}

internal void R_FrameParamsUploadObjects(const R_FramePacket *packet, G_RingBuffer *upload, R_FrameParams *out)
{
	out->object_count = packet->object_count;

	if (packet->skinning_matrix_count > 0)
		out->skinning_palette_buffer = G_RingBufferPushCopyArray(upload, m4, packet->skinning_matrices, packet->skinning_matrix_count);

	out->object_buffer = G_RingBufferPushArray(upload, R_GPU_ObjectData, MaxValue(packet->object_count, 1));

	R_GPU_ObjectData *mapped = out->object_buffer.cpu;

	for (u32 i = 0; mapped && i < packet->object_count; i++)
	{
		const R_PacketObject *object = &packet->objects[i];

		mapped[i].model_matrix = object->transform;
		mapped[i].normal_matrix = object->normal_matrix;
		mapped[i].local_sphere_bounds = object->local_sphere_bounds;
		mapped[i].material_index = object->material_index;
		mapped[i].mesh_index = object->mesh_index;
		mapped[i].page_index = object->page_index;
		mapped[i].skinning_joint_count = object->skinning_joint_count;
		mapped[i].skinning_palette_buffer = 0;

		if (object->skinning_joint_count > 0)
			mapped[i].skinning_palette_buffer = out->skinning_palette_buffer.gpu + object->skinning_first_matrix * sizeof(m4);
	}
}

internal void R_FrameParamsUploadLights(const R_FramePacket *packet, G_RingBuffer *upload, R_FrameParams *out)
{
	out->light_count = packet->light_count;
	out->light_buffer = G_RingBufferPushArray(upload, R_GPU_Light, MaxValue(packet->light_count, 1));

	R_GPU_Light *mapped = out->light_buffer.cpu;

	for (u32 i = 0; mapped && i < packet->light_count; i++)
	{
		const R_Light *light = &packet->lights[i];

		const f32 radius = R_LightHeuristicRadius(light, 0.05f);

		mapped[i].transform = M4Transform(light->position, V4QuatIdentity(), v3x(radius), v3x(0.f));
		mapped[i].position = light->position;
		mapped[i].colour = light->colour;
		mapped[i].intensity = light->intensity;
		mapped[i].attenuation = v3(light->falloff, 0.f, 0.f);
		mapped[i].radius = radius;
		mapped[i].shadow_slot_index = -1;

		if (light->casts_shadows && out->shadow_caster_count < ArraySize(out->shadow_casters))
		{
			mapped[i].shadow_slot_index = (i32)out->shadow_caster_count;

			R_ShadowCaster *caster = &out->shadow_casters[out->shadow_caster_count++];
			caster->position = light->position;
			caster->near = light->shadow_near;
			caster->far = light->shadow_far;
			caster->radius = radius;
		}
	}
}

internal void R_FrameParamsUploadFrameData(const R_FramePacket *packet, G_RingBuffer *upload, R_FrameParams *out)
{
	const R_Camera *camera = &out->camera;

	out->frame_data = G_RingBufferPushArray(upload, R_GPU_FrameData, 1);

	R_GPU_FrameData *frame_data = out->frame_data.cpu;

	if (!frame_data)
		return;

	frame_data->view = camera->view;
	frame_data->proj = camera->proj;
	frame_data->view_proj = camera->view_proj;
	frame_data->view_proj_no_translation = camera->view_proj_no_translation;
	frame_data->inv_view = camera->inv_view;
	frame_data->inv_proj = camera->inv_proj;
	frame_data->camera_position = camera->position;
	frame_data->window_resolution = v2((f32)packet->window_width, (f32)packet->window_height);
	frame_data->frame_number = (u32)out->frame_number;
	frame_data->delta_time = out->dt;
	frame_data->time = out->elapsed;
}

internal b32 R_FrameParamsTryResolveShader(A_Handle handle, G_ResourceKey *out)
{
	A_Record *record = A_GetRecord(handle);

	if (!record || record->load_state != A_LoadState_Ready)
		return false;

	*out = record->asset.shader.key;
	return !G_ResourceKeyIsNull(*out);
}

internal b32 R_FrameParamsResolveShaders(R_System *system, R_FrameParams *out)
{
	b32 ok = true;

	ok &= R_FrameParamsTryResolveShader(system->shaders.debug_line_handle,             &out->debug_line_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.forward_handle,                &out->forward_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.shadow_handle,                 &out->shadow_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.cull_frustum_handle,           &out->cull_frustum_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.cull_sphere_handle,            &out->cull_sphere_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.skybox_handle,                 &out->skybox_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.tonemapping_handle,            &out->tonemapping_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.brdf_lut_generation_handle,    &out->brdf_lut_generation_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.hdr_to_cubemap_handle,         &out->hdr_to_cubemap_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.irradiance_cubemap_gen_handle, &out->irradiance_cubemap_gen_shader);
	ok &= R_FrameParamsTryResolveShader(system->shaders.prefilter_cubemap_gen_handle,  &out->prefilter_cubemap_gen_shader);

	return ok;
}

typedef struct R_TranslucentSortKey R_TranslucentSortKey;
struct R_TranslucentSortKey
{
	f32 depth;
	u32 index;
};

internal void R_TranslucentSortKeysFarToNear(R_TranslucentSortKey *keys, u32 count)
{
	// shell sort
	// todo: is this actually good enough - test

	static const u32 gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1 };

	for (u32 g = 0; g < ArraySize(gaps); g++)
	{
		u32 gap = gaps[g];

		for (u32 i = gap; i < count; i++)
		{
			R_TranslucentSortKey key = keys[i];
			u32 j = i;

			while (j >= gap && keys[j - gap].depth < key.depth)
			{
				keys[j] = keys[j - gap];
				j -= gap;
			}

			keys[j] = key;
		}
	}
}

internal b32 R_FrustumTestSphere(const R_FrustumVolume *frustum, v3 centre, f32 radius)
{
	for (u32 i = 0; i < 6; i++)
	{
		v4 p = frustum->planes[i];

		if (p.x * centre.x + p.y * centre.y + p.z * centre.z + p.w < -radius)
			return false;
	}

	return true;
}

internal void R_FrameParamsBuildTranslucentList(const R_FramePacket *packet, R_FrameParams *out)
{
	Arena *arena = out->arena;

	u32 candidate_count = 0;

	for (u32 i = 0; i < packet->object_count; i++)
		candidate_count += (packet->objects[i].flags & R_ObjectFlag_Translucent) ? 1 : 0;

	out->translucent_count = 0;
	out->translucent_draws = NULL;

	if (candidate_count == 0)
		return;

	R_FrustumVolume frustum = R_CameraFrustum(&out->camera);

	R_TranslucentSortKey *keys = ArenaPushArray(arena, R_TranslucentSortKey, candidate_count);
	u32 key_count = 0;

	for (u32 i = 0; i < packet->object_count; i++)
	{
		const R_PacketObject *object = &packet->objects[i];

		if (!(object->flags & R_ObjectFlag_Translucent))
			continue;

		// same bound as the culling shader.
		// centre through the model matrix radius by the largest axis scale.
		// todo: should this be shared behaviour?
		v4 b = object->local_sphere_bounds;
		v3 centre = M4MulV3Point(object->transform, v3(b.x, b.y, b.z));

		f32 sx = V3Length(v3(object->transform.m00, object->transform.m10, object->transform.m20));
		f32 sy = V3Length(v3(object->transform.m01, object->transform.m11, object->transform.m21));
		f32 sz = V3Length(v3(object->transform.m02, object->transform.m12, object->transform.m22));
		f32 radius = b.w * MaxValue(sx, MaxValue(sy, sz));

		if (!R_FrustumTestSphere(&frustum, centre, radius))
			continue;

		keys[key_count].depth = V3Dot(V3Sub(centre, out->camera.position), out->camera.forward);
		keys[key_count].index = i;
		key_count++;
	}

	R_TranslucentSortKeysFarToNear(keys, key_count);

	out->translucent_draws = ArenaPushArray(arena, R_TranslucentDraw, MaxValue(key_count, 1));

	for (u32 k = 0; k < key_count; k++)
	{
		const R_PacketObject *object = &packet->objects[keys[k].index];

		if (object->mesh_index >= packet->mesh_count)
			continue;

		const R_GPU_RenderMesh *mesh = &packet->meshes[object->mesh_index];

		R_TranslucentDraw *draw = &out->translucent_draws[out->translucent_count++];
		draw->object_index = keys[k].index;
		draw->page_index = object->page_index;
		draw->index_count = mesh->index_count;
		draw->first_index = mesh->first_index;
	}
}

internal R_FrameParams R_FrameParamsBuild(Arena *arena,
										  R_System *system,
										  G_RingBuffer *upload,
										  const R_FramePacket *packet)
{
	R_FrameParams params = {0};

	params.arena = arena;
	params.upload = upload;
	params.packet = packet;
	params.frame_number = packet->frame_number;
	params.dt = packet->dt;
	params.elapsed = packet->elapsed;
	params.camera = packet->camera;

	params.brdf_lut = system->brdf_lut;
	params.irradiance_fallback_cubemap = system->irradiance_cubemap;
	params.prefilter_cubemap = system->prefilter_cubemap;

	params.skybox_mesh = &system->skybox_mesh;

	params.cubemap_capture_transform_buffer = system->cubemap_capture_transform_buffer;

	params.linear_sampler = system->samplers.linear;
	params.nearest_sampler = system->samplers.nearest;

	R_FrameParamsUploadSceneTables(packet, upload, &params);
	R_FrameParamsUploadObjects(packet, upload, &params);
	R_FrameParamsUploadLights(packet, upload, &params);
	R_FrameParamsUploadFrameData(packet, upload, &params);
	R_ShadowsBuildCasterTable(&system->shadow_render_state, upload, &params);
	R_FrameParamsBuildTranslucentList(packet, &params);

	params.shaders_ready = R_FrameParamsResolveShaders(system, &params);

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
							 frame_params->page_index_types[i]);

		G_CmdDrawIndexedIndirectCount(cmd,
									  indirect_buffer,
									  indirect_offset,
									  count_buffer,
									  count_offset,
									  max_draws_per_page,
									  sizeof(R_GPU_IndirectDraw));
	}
}

internal void R_FrameParamsDrawTranslucent(const R_FrameParams *frame_params, G_CmdBuffer *cmd)
{
	u32 bound_page = (u32)-1;

	for (u32 i = 0; i < frame_params->translucent_count; i++)
	{
		const R_TranslucentDraw *draw = &frame_params->translucent_draws[i];

		if (draw->page_index >= frame_params->page_count)
			continue;

		if (draw->page_index != bound_page)
		{
			bound_page = draw->page_index;

			G_CmdBindIndexBuffer(cmd,
								 frame_params->page_index_buffers[bound_page],
								 0, VK_WHOLE_SIZE,
								 frame_params->page_index_types[bound_page]);
		}

		G_CmdDrawIndexed(cmd, draw->index_count, 1, draw->first_index, 0, draw->object_index);
	}
}

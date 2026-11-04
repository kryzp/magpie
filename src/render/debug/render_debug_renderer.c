
static R_DebugRenderer *r_selected_debug_renderer = NULL;

internal R_DebugDrawNode *R_DebugAllocNode(void)
{
	R_DebugDrawNode *node = r_selected_debug_renderer->free_list;

	if (node)
	{
		r_selected_debug_renderer->free_list = node->next;
		MemZeroStruct(node);
	}
	else
	{
		node = ArenaPushArray(r_selected_debug_renderer->arena, R_DebugDrawNode, 1);
	}

	return node;
}

internal void R_DebugFreeNode(R_DebugDrawNode *node)
{
	node->next = r_selected_debug_renderer->free_list;
	r_selected_debug_renderer->free_list = node;
}

internal void R_DebugCreateLineMesh(void)
{
	static const v3 vertices[] = {
		{ 0.f, 0.f, 1.f },
		{ 0.f, 0.f, 0.f },
	};

	static const u16 indices[] = { 0, 1 };

	R_MeshAlloc(&r_selected_debug_renderer->line_mesh,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));

	G_ResourceKey staging = G_StageAlloc(R_MeshVertexBufferSize(&r_selected_debug_renderer->line_mesh) +
										 R_MeshIndexBufferSize(&r_selected_debug_renderer->line_mesh));

	R_MeshWriteToStage(&r_selected_debug_renderer->line_mesh, staging, 0, vertices, indices);

	G_CmdBuffer cmd = G_SubmitImBegin();
	R_MeshUpload(&r_selected_debug_renderer->line_mesh, &cmd, staging, 0);
	G_SubmitImEnd(&cmd);

	G_BufferDestroy(staging);
}

internal void R_DebugCreateCrossMesh(void)
{
	static const v3 vertices[] = {
		{  1.f,  1.f,  1.f },
		{ -1.f, -1.f, -1.f },
		{ -1.f,  1.f,  1.f },
		{  1.f, -1.f, -1.f },
		{  1.f, -1.f,  1.f },
		{ -1.f,  1.f, -1.f },
		{ -1.f, -1.f,  1.f },
		{  1.f,  1.f, -1.f },
	};

	static const u16 indices[] = {
		0, 1,
		2, 3,
		4, 5,
		6, 7
	};

	R_MeshAlloc(&r_selected_debug_renderer->cross_mesh,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));

	G_ResourceKey staging = G_StageAlloc(R_MeshVertexBufferSize(&r_selected_debug_renderer->cross_mesh) +
										 R_MeshIndexBufferSize(&r_selected_debug_renderer->cross_mesh));

	R_MeshWriteToStage(&r_selected_debug_renderer->cross_mesh, staging, 0, vertices, indices);

	G_CmdBuffer cmd = G_SubmitImBegin();
	R_MeshUpload(&r_selected_debug_renderer->cross_mesh, &cmd, staging, 0);
	G_SubmitImEnd(&cmd);

	G_BufferDestroy(staging);
}

internal void R_DebugCreateSphereMesh(void)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	u32 segments = 32;
	u32 vertex_count = segments * 3;
	u32 index_count  = segments * 6;

	v3  *vertices = ArenaPushArray(scratch.arena, v3,  vertex_count);
	u16 *indices  = ArenaPushArray(scratch.arena, u16, index_count);

	u32 v_idx = 0;
	u32 i_idx = 0;

	for (u32 ring = 0; ring < 3; ring++)
	{
		u32 ring_start = v_idx;

		for (u32 i = 0; i < segments; i++)
		{
			f32 angle = ((f32)i / (f32)segments) * (MATH_PI * 2.f);

			f32 c = CosF(angle);
			f32 s = SinF(angle);

			if (ring == 0) vertices[v_idx] = v3(  c,   s, 0.f);
			if (ring == 1) vertices[v_idx] = v3(  c, 0.f,   s);
			if (ring == 2) vertices[v_idx] = v3(0.f,   c,   s);

			indices[i_idx++] = (u16)v_idx;
			indices[i_idx++] = (u16)(ring_start + ((i + 1) % segments));

			v_idx++;
		}
	}

	R_MeshAlloc(&r_selected_debug_renderer->sphere_mesh,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				vertex_count, index_count);

	G_ResourceKey staging = G_StageAlloc(R_MeshVertexBufferSize(&r_selected_debug_renderer->sphere_mesh) +
										 R_MeshIndexBufferSize(&r_selected_debug_renderer->sphere_mesh));

	R_MeshWriteToStage(&r_selected_debug_renderer->sphere_mesh, staging, 0, vertices, indices);

	G_CmdBuffer cmd = G_SubmitImBegin();
	R_MeshUpload(&r_selected_debug_renderer->sphere_mesh, &cmd, staging, 0);
	G_SubmitImEnd(&cmd);

	G_BufferDestroy(staging);

	ScratchRelease(&scratch);
}

internal void R_DebugCreateCircleMesh(void)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	const u32 segments = 32;
	u32 vertex_count = segments;
	u32 index_count = segments * 2;

	v3  *vertices = ArenaPushArray(scratch.arena, v3,  vertex_count);
	u16 *indices  = ArenaPushArray(scratch.arena, u16, index_count);

	for (u32 i = 0; i < segments; i++)
	{
		f32 phi = ((f32)i / (f32)segments) * (MATH_PI * 2.f);
		vertices[i] = V3SphericalToCartesian(1.f, phi, 0.f);

		indices[i*2 + 0] = (u16)i;
		indices[i*2 + 1] = (u16)((i + 1) % segments);
	}

	R_MeshAlloc(&r_selected_debug_renderer->circle_mesh,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				vertex_count, index_count);

	G_ResourceKey staging = G_StageAlloc(R_MeshVertexBufferSize(&r_selected_debug_renderer->circle_mesh) +
										 R_MeshIndexBufferSize(&r_selected_debug_renderer->circle_mesh));

	R_MeshWriteToStage(&r_selected_debug_renderer->circle_mesh, staging, 0, vertices, indices);

	G_CmdBuffer cmd = G_SubmitImBegin();
	R_MeshUpload(&r_selected_debug_renderer->circle_mesh, &cmd, staging, 0);
	G_SubmitImEnd(&cmd);

	G_BufferDestroy(staging);

	ScratchRelease(&scratch);
}

internal void R_DebugCreateCubeMesh(void)
{
	static const v3 vertices[] = {
		{ -1.f, -1.f, -1.f },
		{  1.f, -1.f, -1.f },
		{  1.f,  1.f, -1.f },
		{ -1.f,  1.f, -1.f },
		{ -1.f, -1.f,  1.f },
		{  1.f, -1.f,  1.f },
		{  1.f,  1.f,  1.f },
		{ -1.f,  1.f,  1.f },
	};

	static const u16 indices[] = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7,
	};

	R_MeshAlloc(&r_selected_debug_renderer->cube_mesh,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));

	G_ResourceKey staging = G_StageAlloc(R_MeshVertexBufferSize(&r_selected_debug_renderer->cube_mesh) +
										 R_MeshIndexBufferSize(&r_selected_debug_renderer->cube_mesh));

	R_MeshWriteToStage(&r_selected_debug_renderer->cube_mesh, staging, 0, vertices, indices);

	G_CmdBuffer cmd = G_SubmitImBegin();
	R_MeshUpload(&r_selected_debug_renderer->cube_mesh, &cmd, staging, 0);
	G_SubmitImEnd(&cmd);

	G_BufferDestroy(staging);
}

internal void R_DebugRendererInitAndSelect(R_DebugRenderer *dr, Arena *arena)
{
	dr->arena = arena;
	dr->spinlock = 0;
	
	R_DebugRendererSelect(dr);

	R_DebugCreateLineMesh();
	R_DebugCreateCrossMesh();
	R_DebugCreateSphereMesh();
	R_DebugCreateCircleMesh();
	R_DebugCreateCubeMesh();
}

internal void R_DebugRendererDestroy(void)
{
	R_MeshDestroy(&r_selected_debug_renderer->line_mesh);
	R_MeshDestroy(&r_selected_debug_renderer->cross_mesh);
	R_MeshDestroy(&r_selected_debug_renderer->sphere_mesh);
	R_MeshDestroy(&r_selected_debug_renderer->circle_mesh);
	R_MeshDestroy(&r_selected_debug_renderer->cube_mesh);

	r_selected_debug_renderer = NULL;
}

internal void R_DebugRendererSelect(R_DebugRenderer *dr)
{
	r_selected_debug_renderer = dr;
}

internal void R_DebugPushDrawCall(R_DebugDrawType type,
								  const R_DebugDrawCall *call,
								  b32 depth_enabled)
{
	osapi->SpinLockAcquire(&r_selected_debug_renderer->spinlock);
	
	R_DebugDrawNode *node = R_DebugAllocNode();
	node->type = type;
	node->call = *call;

	R_DebugDrawNode **bucket = depth_enabled
		? &r_selected_debug_renderer->depth_enabled[type]
		: &r_selected_debug_renderer->depth_disabled[type];

	node->next = *bucket;
	*bucket = node;
	
	osapi->SpinLockRelease(&r_selected_debug_renderer->spinlock);
}

internal void R_DebugPushLine(v3 from, v3 to,
							  v4 colour, f32 line_width,
							  f32 duration, b32 depth_enabled)
{
	R_DebugDrawCall call = {0};
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.line.from = from;
	call.line.to = to;

	R_DebugPushDrawCall(R_DebugDrawType_Line, &call, depth_enabled);
}

internal void R_DebugPushCross(v3 point, f32 size,
							   v4 colour, f32 duration, b32 depth_enabled)
{
	R_DebugDrawCall call = {0};
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.cross.point = point;
	call.cross.size = size;

	R_DebugPushDrawCall(R_DebugDrawType_Cross, &call, depth_enabled);
}

internal void R_DebugPushSphere(v3 centre, f32 radius,
								v4 colour, f32 duration, b32 depth_enabled)
{
	R_DebugDrawCall call = {0};
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.sphere.centre = centre;
	call.sphere.radius = radius;

	R_DebugPushDrawCall(R_DebugDrawType_Sphere, &call, depth_enabled);
}

internal void R_DebugPushCircle(v3 centre, f32 radius, v3 plane_normal,
								v4 colour, f32 duration, b32 depth_enabled)
{
	R_DebugDrawCall call = {0};
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.circle.centre = centre;
	call.circle.radius = radius;
	call.circle.normal = plane_normal;

	R_DebugPushDrawCall(R_DebugDrawType_Circle, &call, depth_enabled);
}

internal void R_DebugPushTriangle(v3 a, v3 b, v3 c,
								  v4 colour, f32 line_width,
								  f32 duration, b32 depth_enabled)
{
	R_DebugDrawCall call = {0};
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.triangle.v0 = a;
	call.triangle.v1 = b;
	call.triangle.v2 = c;

	R_DebugPushDrawCall(R_DebugDrawType_Triangle, &call, depth_enabled);
}

internal void R_DebugPushAABB(v3 min, v3 max,
							  v4 colour, f32 line_width,
							  f32 duration, b32 depth_enabled)
{
	R_DebugDrawCall call = {0};
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.aabb.min = min;
	call.aabb.max = max;

	R_DebugPushDrawCall(R_DebugDrawType_AABB, &call, depth_enabled);
}

internal void R_DebugPushOBB(m4 transform, v3 scale,
							 v4 colour, f32 line_width,
							 f32 duration, b32 depth_enabled)
{
	R_DebugDrawCall call = {0};
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.obb.transform = transform;
	call.obb.scale = scale;

	R_DebugPushDrawCall(R_DebugDrawType_OBB, &call, depth_enabled);
}

internal void R_DebugWriteInstance(R_GPU_DebugObjectDraw *draws, u32 *id,
								   m4 transform, v4 colour, f32 thickness, f32 alpha)
{
	if (*id >= R_DEBUG_MAX_DRAWS)
		return;

	v4 premul_colour = v4(colour.x * colour.w,
						  colour.y * colour.w,
						  colour.z * colour.w,
						  colour.w * alpha);

	draws[*id].transform = transform;
	draws[*id].colour    = premul_colour;
	draws[*id].thickness = thickness;

	(*id)++;
}

internal f32 R_DebugCallAlpha(const R_DebugDrawCall *call)
{
	if (call->initial_duration <= MATH_EPSILON_F32)
		return 1.f;

	f32 t = call->duration / call->initial_duration;

	return ClampValue(t, 0.f, 1.f);
}

internal void R_DebugBuildLineInstance(R_GPU_DebugObjectDraw *draws, u32 *id,
									   v3 from, v3 to, v4 colour, f32 thickness, f32 alpha)
{
	//v3 direction = V3Normalize (V3Sub(to, from));
	f32 length = V3Length(V3Sub(to, from));

	m4 transform = M4MulM4(M4Translate(from), M4Scale(v3(length, length, length)));
	// TODO: M4RotateAround for the direction vector.

	R_DebugWriteInstance(draws, id, transform, colour, thickness, alpha);
}

internal void R_DebugBuildInstances(R_DebugRenderer *dr,
									R_DebugDrawNode **buckets,
									R_GPU_DebugObjectDraw *draws,
									u32 *draw_id)
{
	for (u32 type = 0; type < R_DebugDrawType_COUNT; type++)
	{
		for (R_DebugDrawNode *node = buckets[type]; node; node = node->next)
		{
			const R_DebugDrawCall *call = &node->call;
			f32 alpha = R_DebugCallAlpha(call);

			switch (type)
			{
				case R_DebugDrawType_Line:
					{
						R_DebugBuildLineInstance(draws, draw_id,
												 call->line.from, call->line.to,
												 call->colour, call->line_width, alpha);
					}
					break;

				case R_DebugDrawType_Cross:
					{
						f32 half = call->cross.size * 0.5f;
						m4 t = M4MulM4(M4Translate(call->cross.point), M4Scale(v3(half, half, half)));
					
						R_DebugWriteInstance(draws, draw_id, t, call->colour, call->line_width, alpha);
					}
					break;

				case R_DebugDrawType_Sphere:
					{
						f32 r = call->sphere.radius;
						m4 t = M4MulM4(M4Translate(call->sphere.centre), M4Scale(v3(r, r, r)));
					
						R_DebugWriteInstance(draws, draw_id, t, call->colour, call->line_width, alpha);
					}
					break;

				case R_DebugDrawType_Circle:
					{
						f32 r = call->circle.radius;
						// TODO: M4RotateAround for the normal vector.
						m4 t = M4MulM4(M4Translate(call->circle.centre), M4Scale(v3(r, r, r)));
					
						R_DebugWriteInstance(draws, draw_id, t, call->colour, call->line_width, alpha);
					}
					break;

				case R_DebugDrawType_Triangle:
					{
						R_DebugBuildLineInstance(draws, draw_id,
												 call->triangle.v0, call->triangle.v1,
												 call->colour, call->line_width, alpha);
					
						R_DebugBuildLineInstance(draws, draw_id,
												 call->triangle.v1, call->triangle.v2,
												 call->colour, call->line_width, alpha);
					
						R_DebugBuildLineInstance(draws, draw_id,
												 call->triangle.v2, call->triangle.v0,
												 call->colour, call->line_width, alpha);
					}
					break;

				case R_DebugDrawType_AABB:
					{
						v3 centre = V3MulF32(V3Add(call->aabb.max, call->aabb.min), 0.5f);
						v3 size   = V3Sub(call->aabb.max, call->aabb.min);
						m4 t = M4MulM4(M4Translate(centre), M4Scale(size));
						R_DebugWriteInstance(draws, draw_id, t, call->colour, call->line_width, alpha);
					}
					break;

				case R_DebugDrawType_OBB:
					{
						m4 t = M4MulM4(call->obb.transform, M4Scale(call->obb.scale));
						R_DebugWriteInstance(draws, draw_id, t, call->colour, call->line_width, alpha);
					}
					break;
			}
		}
	}
}

internal u32 R_DebugBuildBatches(R_DebugDrawNode **buckets,
								 R_DebugBatch *out_batches,
								 u32 *running_id)
{
	u32 batch_count = 0;

	for (u32 type = 0; type < R_DebugDrawType_COUNT; type++)
	{
		u32 count = 0;

		for (R_DebugDrawNode *node = buckets[type]; node; node = node->next)
		{
			if (type == R_DebugDrawType_Triangle)
				count += 3; // triangles produce 3 line instances
			else
				count += 1;
		}

		if (count == 0)
			continue;

		AssertTrue(batch_count < R_DEBUG_MAX_BATCHES);

		R_DebugBatch *batch = &out_batches[batch_count++];
		batch->type = (R_DebugDrawType)type;
		batch->start = *running_id;
		batch->count = count;

		*running_id += count;
	}

	return batch_count;
}

typedef struct R_DebugPassData R_DebugPassData;
struct R_DebugPassData
{
	const R_FrameParams *frame_params;

	// This frame's instance arrays, in the frame's upload slice.
	u64 instance_address[R_DebugDepthMode_COUNT];
};

internal R_Mesh *R_DebugMeshForType(R_DebugDrawType type)
{
	switch (type)
	{
		case R_DebugDrawType_Line:      return &r_selected_debug_renderer->line_mesh;
		case R_DebugDrawType_Cross:     return &r_selected_debug_renderer->cross_mesh;
		case R_DebugDrawType_Sphere:    return &r_selected_debug_renderer->sphere_mesh;
		case R_DebugDrawType_Circle:    return &r_selected_debug_renderer->circle_mesh;
		case R_DebugDrawType_AABB:      return &r_selected_debug_renderer->cube_mesh;
		case R_DebugDrawType_OBB:       return &r_selected_debug_renderer->cube_mesh;
		case R_DebugDrawType_Triangle:  return &r_selected_debug_renderer->line_mesh;
		default:                        return &r_selected_debug_renderer->line_mesh;
	}
}

internal void R_DebugDrawBatches(G_CmdBuffer *cmd,
								 G_PipelineSt *pipeline_st,
								 const R_DebugBatch *batches,
								 u32 batch_count,
								 u64 instance_address,
								 m4 view_proj)
{
	G_CmdBindPipeline(cmd, pipeline_st->bind_point, pipeline_st->pipeline);

	for (u32 i = 0; i < batch_count; i++)
	{
		const R_DebugBatch *batch = &batches[i];
		R_Mesh *mesh = R_DebugMeshForType((R_DebugDrawType)batch->type);

		struct
		{
			m4 view_proj;
			u64 calls_buffer;
			u64 vertex_buffer;
		}
			args;

		args.view_proj = view_proj;
		args.calls_buffer = instance_address;
		args.vertex_buffer = G_BufferAddress(mesh->vertex_buffer);

		G_CmdPushConstants(cmd, pipeline_st->layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

		R_MeshBindIndexBuffer(mesh, cmd);
		G_CmdDrawIndexed(cmd, mesh->index_count, batch->count, 0, 0, batch->start);
	}
}

internal R_PASS_RECORD_DEF(R_DebugRenderPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	const R_DebugPassData *data = ctx->user_data;
	const R_FrameParams *frame_params = data->frame_params;

	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(frame_params->debug_line_shader, ctx->render_info);
	pipeline_def.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	pipeline_def.cull_mode = VK_CULL_MODE_NONE;
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state. depth_write_enabled = false;
	pipeline_def.blend_state.enabled = true;
	pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_SRC_ALPHA;
	pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
	pipeline_def.blend_state.alpha.src = VK_BLEND_FACTOR_ONE;
	pipeline_def.blend_state.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline_def.blend_state.alpha.op = VK_BLEND_OP_ADD;

	G_PipelineSt pipeline_st = G_FetchGraphicsPipeline(&pipeline_def);

	pipeline_def.depth_stencil_state.depth_test_enabled = false;
	G_PipelineSt pipeline_st_no_depth = G_FetchGraphicsPipeline(&pipeline_def);
	
	m4 view_proj = frame_params->camera.view_proj;
	const R_DebugPacket *debug = &frame_params->packet->debug;
	
	R_DebugDrawBatches(cmd, &pipeline_st,
					   debug->batches[R_DebugDepthMode_Tested],
					   debug->batch_count[R_DebugDepthMode_Tested],
					   data->instance_address[R_DebugDepthMode_Tested],
					   view_proj);

	R_DebugDrawBatches(cmd, &pipeline_st_no_depth,
					   debug->batches[R_DebugDepthMode_Overlay],
					   debug->batch_count[R_DebugDepthMode_Overlay],
					   data->instance_address[R_DebugDepthMode_Overlay],
					   view_proj);
}

internal void R_DebugFilterBuckets(R_DebugRenderer *dr, R_DebugDrawNode **buckets, f32 dt)
{
	for (u32 type = 0; type < R_DebugDrawType_COUNT; type++)
	{
		R_DebugDrawNode **prev = &buckets[type];
		R_DebugDrawNode  *node = *prev;
		
		while (node)
		{
			R_DebugDrawNode *next = node->next;

			if (node->call.duration < 0.f)
			{
				*prev = next;
				R_DebugFreeNode(node);
			}
			else
			{
				node->call.duration -= dt;
				prev = &node->next;
			}

			node = next;
		}
	}
}

internal u32 R_DebugCountInstances(R_DebugDrawNode **buckets)
{
	u32 count = 0;

	for (u32 type = 0; type < R_DebugDrawType_COUNT; type++)
		for (R_DebugDrawNode *node = buckets[type]; node; node = node->next)
			count += (type == R_DebugDrawType_Triangle) ? 3 : 1;

	return MinValue(count, R_DEBUG_MAX_DRAWS);
}

internal void R_DebugCollectInto(R_FramePacket *packet, f32 dt)
{
	R_DebugRenderer *dr = r_selected_debug_renderer;
	R_DebugPacket *out = &packet->debug;

	osapi->SpinLockAcquire(&dr->spinlock);
	
	// Expire old draws and return their nodes to the freelist.
	R_DebugFilterBuckets(dr, dr->depth_enabled, dt);
	R_DebugFilterBuckets(dr, dr->depth_disabled, dt);

	R_DebugDrawNode **buckets[R_DebugDepthMode_COUNT] = {
		dr->depth_enabled,
		dr->depth_disabled,
	};

	for (u32 mode = 0; mode < R_DebugDepthMode_COUNT; mode++)
	{
		u32 count = R_DebugCountInstances(buckets[mode]);
		
		out->instances[mode] = ArenaPushArray(&packet->arena, R_GPU_DebugObjectDraw, MaxValue(count, 1));
		out->instance_count[mode] = 0;

		R_DebugBuildInstances(dr, buckets[mode], out->instances[mode], &out->instance_count[mode]);

		u32 running_id = 0;
		out->batch_count[mode] = R_DebugBuildBatches(buckets[mode], out->batches[mode], &running_id);

		// Keep batches inside what was actually written if we hit the cap.
		for (u32 b = 0; b < out->batch_count[mode]; b++)
		{
			R_DebugBatch *batch = &out->batches[mode][b];
			u32 end = MinValue(batch->start + batch->count, out->instance_count[mode]);
			batch->count = (end > batch->start) ? (end - batch->start) : 0;
		}
	}

	osapi->SpinLockRelease(&dr->spinlock);
}

internal void R_DebugRendererRender(R_Graph *graph,
									const R_FrameParams *frame_params,
									R_GraphTexHandle target_colour,
									R_GraphTexHandle target_depth)
{
	const R_DebugPacket *debug = &frame_params->packet->debug;

	if (debug->instance_count[R_DebugDepthMode_Tested] == 0 &&
		debug->instance_count[R_DebugDepthMode_Overlay] == 0)
	{
		return;
	}

	R_DebugPassData *data = ArenaPushArray(frame_params->arena, R_DebugPassData, 1);
	data->frame_params = frame_params;

	for (u32 mode = 0; mode < R_DebugDepthMode_COUNT; mode++)
	{
		G_Alloc alloc = G_RingBufferPushCopyArray(frame_params->upload,
												  R_GPU_DebugObjectDraw,
												  debug->instances[mode],
												  MaxValue(debug->instance_count[mode], 1));
		data->instance_address[mode] = alloc.gpu;
	}
	
	R_Pass *pass = R_GraphAdd(graph, String8Lit("Debug Lines"), R_PassType_Graphics);
	R_PassSetRecord(pass, R_DebugRenderPassFn, data);
	R_PassWriteColour(pass, target_colour, NULL);
	R_PassWriteDepth(pass, target_depth, NULL);
}

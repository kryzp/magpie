
/*
 * TODO: I was lazy and didn't put a lot of these function
 *       defintions into the header file. Do that at some point!!!
 */

global R_DebugRenderer *r_selected_debug_renderer = NULL;

internal R_DebugDrawNode *
R_DebugAllocNode(R_DebugRenderer *dr)
{
	R_DebugDrawNode *node = dr->free_list;

	if (node)
	{
		dr->free_list = node->next;
		MemZeroStruct(node);
	}
	else
	{
		node = ArenaPushArray(dr->arena, R_DebugDrawNode, 1);
	}

	return node;
}

internal void
R_DebugFreeNode(R_DebugRenderer *dr, R_DebugDrawNode *node)
{
	node->next = dr->free_list;
	dr->free_list = node;
}

internal void
R_DebugCreateLineMesh(R_DebugRenderer *dr)
{
	static const v3 vertices[] = {
		{ 0.f, 0.f, 1.f },
		{ 0.f, 0.f, 0.f },
	};

	static const u16 indices[] = { 0, 1 };

	R_MeshAlloc(&dr->line_mesh, dr->device,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));

	GFX_BufferKey staging = GFX_DeviceStageAlloc(dr->device,
												 R_MeshVertexBufferSize(&dr->line_mesh) +
												 R_MeshIndexBufferSize(&dr->line_mesh));

	R_MeshWriteToStage(&dr->line_mesh, dr->device, staging, 0, vertices, indices);

	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(dr->device);
	R_MeshUpload(&dr->line_mesh, &cmd, staging, 0);
	GFX_DeviceSubmitImEnd(dr->device, &cmd);

	GFX_DeviceBufferDestroy(dr->device, staging);
}

internal void
R_DebugCreateCrossMesh(R_DebugRenderer *dr)
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

	R_MeshAlloc(&dr->cross_mesh, dr->device,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));

	GFX_BufferKey staging = GFX_DeviceStageAlloc(dr->device,
												 R_MeshVertexBufferSize(&dr->cross_mesh) +
												 R_MeshIndexBufferSize(&dr->cross_mesh));

	R_MeshWriteToStage(&dr->cross_mesh, dr->device, staging, 0, vertices, indices);

	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(dr->device);
	R_MeshUpload(&dr->cross_mesh, &cmd, staging, 0);
	GFX_DeviceSubmitImEnd(dr->device, &cmd);

	GFX_DeviceBufferDestroy(dr->device, staging);
}

internal void
R_DebugCreateSphereMesh(R_DebugRenderer *dr)
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

	R_MeshAlloc(&dr->sphere_mesh, dr->device,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				vertex_count, index_count);

	GFX_BufferKey staging = GFX_DeviceStageAlloc(dr->device,
												 R_MeshVertexBufferSize(&dr->sphere_mesh) +
												 R_MeshIndexBufferSize(&dr->sphere_mesh));

	R_MeshWriteToStage(&dr->sphere_mesh, dr->device, staging, 0, vertices, indices);

	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(dr->device);
	R_MeshUpload(&dr->sphere_mesh, &cmd, staging, 0);
	GFX_DeviceSubmitImEnd(dr->device, &cmd);

	GFX_DeviceBufferDestroy(dr->device, staging);

	ScratchRelease(&scratch);
}

internal void
R_DebugCreateCircleMesh(R_DebugRenderer *dr)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	u32 segments     = 32;
	u32 vertex_count = segments;
	u32 index_count  = segments * 2;

	v3  *vertices = ArenaPushArray(scratch.arena, v3,  vertex_count);
	u16 *indices  = ArenaPushArray(scratch.arena, u16, index_count);

	for (u32 i = 0; i < segments; i++)
	{
		f32 phi = ((f32)i / (f32)segments) * (MATH_PI * 2.f);
		vertices[i] = V3SphericalToCartesian(1.f, phi, 0.f);

		indices[i*2 + 0] = (u16)i;
		indices[i*2 + 1] = (u16)((i + 1) % segments);
	}

	R_MeshAlloc(&dr->circle_mesh, dr->device,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				vertex_count, index_count);

	GFX_BufferKey staging = GFX_DeviceStageAlloc(dr->device,
												 R_MeshVertexBufferSize(&dr->circle_mesh) +
												 R_MeshIndexBufferSize(&dr->circle_mesh));

	R_MeshWriteToStage(&dr->circle_mesh, dr->device, staging, 0, vertices, indices);

	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(dr->device);
	R_MeshUpload(&dr->circle_mesh, &cmd, staging, 0);
	GFX_DeviceSubmitImEnd(dr->device, &cmd);

	GFX_DeviceBufferDestroy(dr->device, staging);

	ScratchRelease(&scratch);
}

internal void
R_DebugCreateCubeMesh(R_DebugRenderer *dr)
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
		0, 1,  1, 2,  2, 3,  3, 0,
		4, 5,  5, 6,  6, 7,  7, 4,
		0, 4,  1, 5,  2, 6,  3, 7,
	};

	R_MeshAlloc(&dr->cube_mesh, dr->device,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));

	GFX_BufferKey staging = GFX_DeviceStageAlloc(dr->device,
												 R_MeshVertexBufferSize(&dr->cube_mesh) +
												 R_MeshIndexBufferSize(&dr->cube_mesh));

	R_MeshWriteToStage(&dr->cube_mesh, dr->device, staging, 0, vertices, indices);

	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(dr->device);
	R_MeshUpload(&dr->cube_mesh, &cmd, staging, 0);
	GFX_DeviceSubmitImEnd(dr->device, &cmd);

	GFX_DeviceBufferDestroy(dr->device, staging);
}

internal void
R_DebugRendererInitAndSelect(R_DebugRenderer *dr, Arena *arena, GFX_Device *device, AST_Assets *assets)
{
	MemZeroStruct(dr);

	dr->arena  = arena;
	dr->device = device;
	dr->assets = assets;

	dr->shader_handle = AST_Require(assets, String8Lit("assets://shaders/passes/debug/debug_line.slang"), AST_Type_Shader);

	GFX_BufferAllocInfo buf_info = {0};
	buf_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	buf_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	buf_info.size  = sizeof(R_GPU_DebugObjectDraw) * R_DEBUG_MAX_DRAWS;

	dr->depth_enabled_buffer  = GFX_DeviceBufferAlloc(device, &buf_info);
	dr->depth_disabled_buffer = GFX_DeviceBufferAlloc(device, &buf_info);

	R_DebugCreateLineMesh   (dr);
	R_DebugCreateCrossMesh  (dr);
	R_DebugCreateSphereMesh (dr);
	R_DebugCreateCircleMesh (dr);
	R_DebugCreateCubeMesh   (dr);
	
	r_selected_debug_renderer = dr;
}

internal void
R_DebugRendererDestroy(R_DebugRenderer *dr)
{
	R_MeshDestroy(&dr->line_mesh,   dr->device);
	R_MeshDestroy(&dr->cross_mesh,  dr->device);
	R_MeshDestroy(&dr->sphere_mesh, dr->device);
	R_MeshDestroy(&dr->circle_mesh, dr->device);
	R_MeshDestroy(&dr->cube_mesh,   dr->device);

	GFX_DeviceBufferDestroy(dr->device, dr->depth_enabled_buffer);
	GFX_DeviceBufferDestroy(dr->device, dr->depth_disabled_buffer);
}

internal void
R_DebugRendererSelect(R_DebugRenderer *dr)
{
	r_selected_debug_renderer = dr;
}

internal void
R_DebugPushDrawCall(R_DebugDrawType type,
					const R_DebugDrawCall *call,
					b32 depth_enabled)
{
	R_DebugDrawNode *node = R_DebugAllocNode(r_selected_debug_renderer);
	node->type = type;
	node->call = *call;

	R_DebugDrawNode **bucket = depth_enabled
		? &r_selected_debug_renderer->depth_enabled[type]
		: &r_selected_debug_renderer->depth_disabled[type];

	node->next = *bucket;
	*bucket = node;
}

internal void
R_DebugPushLine(v3 from, v3 to,
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

internal void
R_DebugPushCross(v3 point, f32 size,
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

internal void
R_DebugPushSphere(v3 centre, f32 radius,
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

internal void
R_DebugPushCircle(v3 centre, f32 radius, v3 plane_normal,
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

internal void
R_DebugPushTriangle(v3 a, v3 b, v3 c,
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

internal void
R_DebugPushAABB(v3 min, v3 max,
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

internal void
R_DebugPushOBB(m4 transform, v3 scale,
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

internal void
R_DebugWriteInstance(R_GPU_DebugObjectDraw *draws, u32 *id,
					 m4 transform, v4 colour, f32 thickness, f32 alpha)
{
	AssertTrue(*id < R_DEBUG_MAX_DRAWS);

	v4 premul_colour = v4(colour.x * colour.w,
						  colour.y * colour.w,
						  colour.z * colour.w,
						  colour.w * alpha);

	draws[*id].transform = transform;
	draws[*id].colour    = premul_colour;
	draws[*id].thickness = thickness;

	(*id)++;
}

internal f32
R_DebugCallAlpha(const R_DebugDrawCall *call)
{
	if (call->initial_duration <= MATH_EPSILON_F32)
		return 1.f;

	f32 t = call->duration / call->initial_duration;

	return ClampValue(t, 0.f, 1.f);
}

internal void
R_DebugBuildLineInstance(R_GPU_DebugObjectDraw *draws, u32 *id,
						 v3 from, v3 to, v4 colour, f32 thickness, f32 alpha)
{
	v3 direction = V3Normalize (V3Sub(to, from));
	f32 length   = V3Length    (V3Sub(to, from));

	m4 transform = M4MulM4(M4Translate(from), M4Scale(v3(length, length, length)));
	// TODO: M4RotateAround for the direction vector.

	R_DebugWriteInstance(draws, id, transform, colour, thickness, alpha);
}

internal void
R_DebugBuildInstances(R_DebugRenderer *dr,
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

typedef struct R_DebugBatch R_DebugBatch;
struct R_DebugBatch
{
	R_DebugDrawType type;
	u32 start;
	u32 count;
};

internal u32
R_DebugBuildBatches(R_DebugDrawNode **buckets,
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
		batch->type  = (R_DebugDrawType)type;
		batch->start = *running_id;
		batch->count = count;

		*running_id += count;
	}

	return batch_count;
}

typedef struct R_DebugPassData R_DebugPassData;
struct R_DebugPassData
{
	GFX_ShaderKey shader;

	GFX_BufferKey depth_enabled_buffer;
	GFX_BufferKey depth_disabled_buffer;

	u32 depth_batch_count;
	R_DebugBatch depth_batches[R_DEBUG_MAX_BATCHES];

	u32 no_depth_batch_count;
	R_DebugBatch no_depth_batches[R_DEBUG_MAX_BATCHES];

	R_Mesh *line_mesh;
	R_Mesh *cross_mesh;
	R_Mesh *sphere_mesh;
	R_Mesh *circle_mesh;
	R_Mesh *cube_mesh;
};

internal R_Mesh *
R_DebugMeshForType(const R_DebugPassData *data, R_DebugDrawType type)
{
	switch (type)
	{
		case R_DebugDrawType_Line:      return data->line_mesh;
		case R_DebugDrawType_Cross:     return data->cross_mesh;
		case R_DebugDrawType_Sphere:    return data->sphere_mesh;
		case R_DebugDrawType_Circle:    return data->circle_mesh;
		case R_DebugDrawType_AABB:      return data->cube_mesh;
		case R_DebugDrawType_OBB:       return data->cube_mesh;
		case R_DebugDrawType_Triangle:  return data->line_mesh;
		default:                        return data->line_mesh;
	}
}

internal void
R_DebugDrawBatches(const R_DebugPassData *data,
				   GFX_Device *device,
				   GFX_CmdBuffer *cmd,
				   GFX_PipelineSt *pipeline_st,
				   const R_DebugBatch *batches,
				   u32 batch_count,
				   GFX_BufferKey buffer,
				   m4 view_proj)
{
	GFX_CmdBindPipeline(cmd, pipeline_st->bind_point, pipeline_st->pipeline);

	for (u32 i = 0; i < batch_count; i++)
	{
		const R_DebugBatch *batch = &batches[i];
		R_Mesh *mesh = R_DebugMeshForType(data, batch->type);

		struct
		{
			m4  view_proj;
			u64 calls_buffer;
			u64 vertex_buffer;
		}
		args;

		args.view_proj     = view_proj;
		args.calls_buffer  = GFX_DeviceBufferAddress(device, buffer);
		args.vertex_buffer = GFX_DeviceBufferAddress(device, mesh->vertex_buffer);

		GFX_CmdPushConstants(cmd, pipeline_st->layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);

		R_MeshBind(mesh, cmd);
		GFX_CmdDrawIndexed(cmd, mesh->index_count, batch->count, 0, 0, batch->start);
	}
}

R_PASS_RECORD_DEF(R_DebugRenderPassFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;

	const R_DebugPassData *data = ctx->user_data;

	
	// Pipeline with depth testing.

	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefFromInfo(data->shader, ctx->render_info);
	pipeline_def.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	pipeline_def.cull_mode = VK_CULL_MODE_NONE;
	pipeline_def.depth_stencil_state.depth_test_enabled  = true;
	pipeline_def.depth_stencil_state. depth_write_enabled = false;
	pipeline_def.blend_state.enabled = true;
	pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_SRC_ALPHA;
	pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
	pipeline_def.blend_state.alpha.src = VK_BLEND_FACTOR_ONE;
	pipeline_def.blend_state.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline_def.blend_state.alpha.op = VK_BLEND_OP_ADD;

	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);

	
	// Pipeline without depth testing.

	pipeline_def.depth_stencil_state.depth_test_enabled = false;
	GFX_PipelineSt pipeline_st_no_depth = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);

	
	// Draw.

	m4 view_proj = M4MulM4(ctx->camera->proj, ctx->camera->view); // TODO: the debug lines appear to be lagging behind the camera what gives?

	R_DebugDrawBatches(data, device, cmd, &pipeline_st,
					   data->depth_batches, data->depth_batch_count,
					   data->depth_enabled_buffer, view_proj);

	R_DebugDrawBatches(data, device, cmd, &pipeline_st_no_depth,
					   data->no_depth_batches, data->no_depth_batch_count,
					   data->depth_disabled_buffer, view_proj);
}

internal void
R_DebugFilterBuckets(R_DebugRenderer *dr, R_DebugDrawNode **buckets, f32 dt)
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
				R_DebugFreeNode(dr, node);
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

internal void
R_DebugRendererRender(R_DebugRenderer *dr,
					  f32 dt,
					  R_Graph *graph,
					  Arena *pass_arena,
					  R_GraphTexHandle target_colour,
					  R_GraphTexHandle target_depth)
{
	// Expire old draws and return their nodes to the freelist.

	R_DebugFilterBuckets(dr, dr->depth_enabled,  dt);
	R_DebugFilterBuckets(dr, dr->depth_disabled, dt);

	
	// Build GPU instance data for both depth modes.

	u32 depth_enabled_id = 0;
	u32 depth_disabled_id = 0;

	R_GPU_DebugObjectDraw *depth_enabled_draws  = GFX_DeviceBufferMap(dr->device, dr->depth_enabled_buffer);
	R_GPU_DebugObjectDraw *depth_disabled_draws = GFX_DeviceBufferMap(dr->device, dr->depth_disabled_buffer);

	R_DebugBuildInstances(dr, dr->depth_enabled,  depth_enabled_draws,  &depth_enabled_id);
	R_DebugBuildInstances(dr, dr->depth_disabled, depth_disabled_draws, &depth_disabled_id);

	
	// Build batch descriptors.

	u32    depth_running_id = 0;
	u32 no_depth_running_id = 0;

	R_DebugBatch    depth_batches[R_DEBUG_MAX_BATCHES] = {0};
	R_DebugBatch no_depth_batches[R_DEBUG_MAX_BATCHES] = {0};

	u32    depth_batch_count = R_DebugBuildBatches(dr->depth_enabled,  depth_batches,    &depth_running_id);
	u32 no_depth_batch_count = R_DebugBuildBatches(dr->depth_disabled, no_depth_batches, &no_depth_running_id);

	
	// Create the render pass.

	GFX_ShaderKey shader = AST_GetNow(dr->assets, dr->shader_handle, AST_Type_Shader)->shader.key;

	R_DebugPassData *data = ArenaPushArray(pass_arena, R_DebugPassData, 1);
	data->shader = shader;
	data->depth_enabled_buffer = dr->depth_enabled_buffer;
	data->depth_disabled_buffer = dr->depth_disabled_buffer;
	data->line_mesh = &dr->line_mesh;
	data->cross_mesh = &dr->cross_mesh;
	data->sphere_mesh = &dr->sphere_mesh;
	data->circle_mesh = &dr->circle_mesh;
	data->cube_mesh = &dr->cube_mesh;

	data->depth_batch_count    =    depth_batch_count;
	data->no_depth_batch_count = no_depth_batch_count;

	MemCopy(data->depth_batches,    depth_batches,    depth_batch_count    * sizeof(R_DebugBatch));
	MemCopy(data->no_depth_batches, no_depth_batches, no_depth_batch_count * sizeof(R_DebugBatch));

	R_Pass *pass = R_GraphAdd(graph, String8Lit("Debug Lines"), R_PassType_Graphics);
	R_PassSetRecord(pass, R_DebugRenderPassFn, data);
	R_PassWriteColour(pass, target_colour, NULL);
	R_PassWriteDepth(pass, target_depth, NULL);
}

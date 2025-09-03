

internal RenderStateFrameData *RenderStateGetCurrentFrameData(RenderState *rs)
{
	return rs->per_frame_data + graphics_device->current_frame_index;
}

internal void RenderStateCreatePerFrameObjects(RenderState *st)
{
	for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
		RenderStateFrameData *frame = st->per_frame_data + i;

		frame->frame_data_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
							  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							  sizeof(GPU_FrameData));
		
		frame->object_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
						      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						      sizeof(GPU_ObjectData) * SCENE_MAX_OBJECTS);

		frame->light_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
						     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						     sizeof(GPU_Light) * SCENE_MAX_OBJECTS);

		frame->indirect_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
							VK_BUFFER_USAGE_TRANSFER_DST_BIT |
							VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
							VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							sizeof(VkDrawIndexedIndirectCommand) *
							SCENE_MAX_OBJECTS);
	}
}

internal void RenderStateDestroyPerFrameObjects(RenderState *rs)
{
	for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
		RenderStateFrameData *frame = rs->per_frame_data + i;

		GPUBufferDestroy(&frame->frame_data_buffer);
		GPUBufferDestroy(&frame->object_buffer);
		GPUBufferDestroy(&frame->light_buffer);
		GPUBufferDestroy(&frame->indirect_buffer);
	}
}

internal void RenderStateInit(RenderState *rs)
{
	RenderStateCreatePerFrameObjects(rs);
	
	rs->material_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
					     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
					     sizeof(GPU_Material) * SCENE_MAX_MATERIALS);
}

internal void RenderStateDestroy(RenderState *rs)
{
	RenderStateDestroyPerFrameObjects(rs);
	
	GPUBufferDestroy(&rs->material_buffer);

	GPUBufferDestroy(&rs->merged_vertex_buffer);
	GPUBufferDestroy(&rs->merged_index_buffer);
}

internal u32 RenderStateUploadMesh(RenderState *rs, Mesh *mesh)
{
	for (i32 i = 0; i < rs->mesh_count; i++) {
		if (MeshesEqual(rs->meshes[i].original, mesh))
			return i;
	}

	PassMesh pass_mesh = {0};
	{
		pass_mesh.original = mesh;
		pass_mesh.is_merged = false;
		pass_mesh.first_vertex = 0;
		pass_mesh.first_index = 0;
		pass_mesh.vertex_count = mesh->vertex_count;
		pass_mesh.index_count = mesh->index_count;
	}

	rs->meshes[rs->mesh_count] = pass_mesh;

	return rs->mesh_count++;
}

internal u32 RenderStateUploadMaterial(RenderState *rs, Assets *assets, Material *material)
{
	for (i32 i = 0; i < rs->material_count; i++) {
		if (MaterialsEqual(&rs->materials[i], material))
			return i;
	}

	rs->materials[rs->material_count] = *material;
	
	GPU_Material gpu_material = {0};
	gpu_material.diffuse_texture            = FetchStandardImageView(AssetsImageFromHandle(assets, material->diffuse_texture_handle))->resource_id;
	gpu_material.normal_texture             = FetchStandardImageView(AssetsImageFromHandle(assets, material->normal_texture_handle))->resource_id;
	gpu_material.emissive_texture           = FetchStandardImageView(AssetsImageFromHandle(assets, material->emissive_texture_handle))->resource_id;
	gpu_material.metallic_roughness_texture = FetchStandardImageView(AssetsImageFromHandle(assets, material->metallic_roughness_texture_handle))->resource_id;
	gpu_material.ambient_texture            = FetchStandardImageView(AssetsImageFromHandle(assets, material->ambient_texture_handle))->resource_id;

	GPUBufferWrite(&rs->material_buffer, &gpu_material,
		       sizeof(GPU_Material),
		       sizeof(GPU_Material) * rs->material_count);

	return rs->material_count++;
}

internal u32 RenderStateMakeLight(RenderState *rs, Light *light)
{
	rs->lights[rs->light_count] = *light;
	return rs->light_count++;
}

internal void RenderStateMergeMeshes(RenderState *rs)
{
	if (rs->mesh_count <= 0)
		return;

	// All meshes in the list *should* have the same vertex format.
	// If they don't we have a bit of a problem :/.
	VertexFormat *vertex_format = rs->meshes->original->vertex_format;

	u32 total_vertices = 0;
	u32 total_indices = 0;

	for (i32 i = 0; i < rs->mesh_count; i++) {
		PassMesh *mesh = &rs->meshes[i];

		mesh->first_vertex = total_vertices;
		mesh->first_index = total_indices;

		total_vertices += mesh->vertex_count;
		total_indices += mesh->index_count;

		mesh->is_merged = true;
	}

	u64 vb_size = total_vertices * vertex_format->vertex_size;
	u64 ib_size  = total_indices * sizeof(u16);
	
	if (rs->merged_vertex_buffer.size < vb_size ||
	    rs->merged_index_buffer.size < ib_size) {
		
		GPUBufferDestroy(&rs->merged_vertex_buffer);
		GPUBufferDestroy(&rs->merged_index_buffer);
		
		rs->merged_vertex_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
							  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							  vb_size);
	
		rs->merged_index_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
							 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							 ib_size);
	}
	
	CommandBuffer cmd = BeginGraphicsInstantSubmit();

	for (i32 i = 0; i < rs->mesh_count; i++) {
		PassMesh *mesh = &rs->meshes[i];

		VkBufferCopy vertex_copy = {0};
		vertex_copy.srcOffset = 0;
		vertex_copy.dstOffset = mesh->first_vertex * vertex_format->vertex_size;
		vertex_copy.size      = mesh->vertex_count * vertex_format->vertex_size;

		CmdCopyBufferToBuffer(&cmd,
				      &mesh->original->vertex_buffer,
				      &rs->merged_vertex_buffer,
				      1, &vertex_copy);
		
		VkBufferCopy index_copy = {0};
		index_copy.srcOffset = 0;
		index_copy.dstOffset = mesh->first_index * sizeof(u16);
		index_copy.size      = mesh->index_count * sizeof(u16);

		CmdCopyBufferToBuffer(&cmd,
				      &mesh->original->index_buffer,
				      &rs->merged_index_buffer,
				      1, &index_copy);
	}

	EndGraphicsInstantSubmit(&cmd);
}

internal IndirectBatch *MeshPassCompactDrawsToBatches(MeshPass *pass,
						      Scene *scene,
						      MemoryArena *arena)
{
	SceneObject *render_object = SceneObjectFromHandle(scene, pass->direct_batches->object_id);

	IndirectBatch *batch = MemoryArenaPush(arena, sizeof(IndirectBatch));
	batch->mesh_id = render_object->mesh_id;
	batch->material_id = render_object->material_id;
	batch->first = 0;
	batch->count = 1;

	u32 i = 1;
	for (DirectBatch *curr = pass->direct_batches->next; curr; curr = curr->next, i++) {
		
		render_object = SceneObjectFromHandle(scene, curr->object_id);

		b32 are_same_mesh     = render_object->mesh_id     == batch->mesh_id;
		b32 are_same_material = render_object->material_id == batch->material_id;

		if (are_same_mesh && are_same_material) {
			batch->count++;
		} else {
			IndirectBatch *new_batch = MemoryArenaPush(arena, sizeof(IndirectBatch));
			new_batch->next = batch;
			new_batch->mesh_id = render_object->mesh_id;
			new_batch->material_id = render_object->material_id;
			new_batch->first = i;
			new_batch->count = 1u;

			batch = new_batch;
		}
	}

	return batch;
}

internal void PopulateMeshPass(MeshPass *pass,
			       RenderState *rs,
			       MemoryArena *arena,
			       Scene *scene)
{
	// RENDER BATCHES.
	{
		pass->direct_batches = NULL;
		
		for (SceneObject *s = scene->objects; s; s = s->next) {
			if (s->mesh_id == SCENE_INVALID_HANDLE)
				continue;

			DirectBatch *direct_batch = MemoryArenaPush(arena, sizeof(DirectBatch));
			direct_batch->next = pass->direct_batches;
			direct_batch->object_id = s->id;
			pass->direct_batches = direct_batch;
		}
	}

	// BATCHES.
	{
		pass->batches = MeshPassCompactDrawsToBatches(pass, scene, arena);
	}

	// MULTI BATCHES.
	{
		MultiBatch *multi_batch = MemoryArenaPush(arena, sizeof(MultiBatch));
		multi_batch->next = 0;
		multi_batch->count = 1;
		multi_batch->first = 0;
		pass->multi_batches = multi_batch;
		
		u32 i = 1;
		for (IndirectBatch *batch = pass->batches->next; batch; batch = batch->next, i++)
		{
			// Iterate up to the (multi_batch->first)'th batch.
			IndirectBatch *join_batch = pass->batches;
			for (i32 j = 0; j < multi_batch->first; j++, join_batch = join_batch->next);

			b32 compatible_mesh = rs->meshes[join_batch->mesh_id].is_merged;
			b32 same_material = join_batch->material_id == batch->material_id;
			
			// As long as the materials are the same and the mesh
			// has been merged then we can combine the rendering
			// calls together.
			if (compatible_mesh && same_material)
			{
				multi_batch->count++;
			}
			else
			{
				multi_batch = MemoryArenaPush(arena, sizeof(MultiBatch));
				multi_batch->next = pass->multi_batches;
				multi_batch->count = 1u;
				multi_batch->first = i;
				pass->multi_batches = multi_batch;
			}
		}
	}
}

internal void RenderStateUpdate(RenderState *rs,
				MemoryArena *arena,
				Scene *scene,
				Camera *camera)
{
	RenderStateFrameData *current_frame = RenderStateGetCurrentFrameData(rs);
	
	RenderStateMergeMeshes(rs);
	PopulateMeshPass(&rs->mesh_pass, rs, arena, scene);
	
	i32 mesh_count = 0;
	i32 light_count = 0;

	for (SceneObject *object = scene->objects; object; object = object->next) {

		// If the object has a mesh.
		if (object->mesh_id != SCENE_INVALID_HANDLE) {
			GPU_ObjectData object_data = {0};
			object_data.model_matrix = object->transform;
			object_data.normal_matrix = M4Inverse(M4Transpose(object->transform));

			GPUBufferWrite(&current_frame->object_buffer,
				       &object_data,
				       sizeof(GPU_ObjectData),
				       sizeof(GPU_ObjectData) * mesh_count);

			VkDrawIndexedIndirectCommand command = {0};
			command.indexCount = rs->meshes[object->mesh_id].index_count;
			command.instanceCount = 1;
			command.firstIndex = 0;
			command.vertexOffset = 0;
			command.firstInstance = mesh_count;

			GPUBufferWrite(&current_frame->indirect_buffer,
				       &command,
				       sizeof(VkDrawIndexedIndirectCommand),
				       sizeof(VkDrawIndexedIndirectCommand) * mesh_count);

			mesh_count++;
		}

		// If the object has a light.
		if (object->light_id != SCENE_INVALID_HANDLE) {
			Light *light = &rs->lights[object->light_id];
			
			f32 epsilon_intensity = .1f;
			f32 light_max = V3MaxValue(light->colour);
			f32 heuristic_radius = SquareRoot((light->intensity * light_max) / (light->falloff * epsilon_intensity));

			GPU_Light gpu_light = {0};
			gpu_light.position     = object->transform.c3; // Last column of transformation matrix is translation.
			gpu_light.colour.xyz   = light->colour;
			gpu_light.colour.w     = light->intensity;
			gpu_light.attenuation  = v4(light->falloff, 0.f, 0.f, 0.f);
			gpu_light.transform    = M4Transform(gpu_light.position.xyz,
							     QuatInitIdentity(),
							     v3u(heuristic_radius),
							     v3u(0.f));

			GPUBufferWrite(&current_frame->light_buffer,
				       &gpu_light,
				       sizeof(GPU_Light),
				       sizeof(GPU_Light) * light_count);

			light_count++;
		}

	}

	// Per-frame buffer
	GPU_FrameData frame_data = {0};
	frame_data.view = camera->view;
	frame_data.projection = camera->projection;
	frame_data.view_projection = M4MultiplyM4(frame_data.projection, frame_data.view);
	frame_data.view_projection_no_translation = M4MultiplyM4(frame_data.projection, M4RemoveTranslation(frame_data.view));
	frame_data.inv_view = M4Inverse(frame_data.view);
	frame_data.inv_projection = M4Inverse(frame_data.projection);
	frame_data.camera_position.xyz = camera->position;
	frame_data.window_resolution.x = platform->window_pixel_width;
	frame_data.window_resolution.y = platform->window_pixel_height;
	frame_data.time = GetTotalElapsedSecondsF();

	GPUBufferWrite(&current_frame->frame_data_buffer,
		       &frame_data,
		       sizeof(GPU_FrameData), 0);
}

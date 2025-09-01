
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
}

internal u32 RenderStateUploadMesh(RenderState *rs, Mesh *mesh)
{
	for (i32 i = 0; i < rs->mesh_count; i++) {
		if (MeshesEqual(rs->meshes[i].original, mesh))
			return i;
	}

	RenderMesh render_mesh = {0};
	{
		render_mesh.original = mesh;
		render_mesh.is_merged = false;
		render_mesh.first_vertex = 0;
		render_mesh.first_index = 0;
		render_mesh.index_count = mesh->index_count;
	}

	rs->meshes[rs->mesh_count] = render_mesh;

	return rs->mesh_count++;
}

internal u32 RenderStateUploadMaterial(RenderState *rs,
				       Assets *assets, Material *material)
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

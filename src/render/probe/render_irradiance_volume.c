
internal void
R_IrradianceVolumeInit(R_IrradianceVolume *vol,
					   G_Device *device, A_Registry *assets,
					   LOG_Channel log_channel,
					   v3 grid_min, v3 grid_max,
					   u32 nx, u32 ny, u32 nz,
					   const R_Mesh *skybox_mesh,
					   G_TextureViewKey environment_view,
					   G_SamplerKey linear_sampler)
{
	vol->device = device;
	vol->assets = assets;

	vol->log_channel = log_channel;

	vol->grid_min = grid_min;
	vol->grid_max = grid_max;

	vol->nx     = nx;
	vol->ny     = ny;
	vol->nz     = nz;
	vol->ntotal = nx * ny * nz;

	vol->skybox_mesh      = skybox_mesh;
	vol->environment_view = environment_view;
	vol->linear_sampler   = linear_sampler;

	// Allocate SH coeffient buffer.
	{
		G_BufferAllocInfo info = {0};
		info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
		info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		info.size  = sizeof(R_GPU_ProbeSH) * vol->ntotal;

		vol->sh_buffer = G_DeviceBufferAlloc(device, &info);
	}

	// Allocate and upload the grid info buffer.
	{
		G_BufferAllocInfo info = {0};
		info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
		info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		info.size = sizeof(R_GPU_ProbeGridInfo);

		vol->grid_info_buffer = G_DeviceBufferAlloc(device, &info);

		R_GPU_ProbeGridInfo grid_info = {0};
		grid_info.grid_min = grid_min;
		grid_info.grid_max = grid_max;
		grid_info.nx = nx;
		grid_info.ny = ny;
		grid_info.nz = nz;
		grid_info.ntotal = vol->ntotal;

		grid_info.cell_size = v3((nx > 1) ? (grid_max.x - grid_min.x) / (f32)(nx - 1) : 0.f,
								 (ny > 1) ? (grid_max.y - grid_min.y) / (f32)(ny - 1) : 0.f,
								 (nz > 1) ? (grid_max.z - grid_min.z) / (f32)(nz - 1) : 0.f);

		G_DeviceBufferWrite(device, vol->grid_info_buffer,
							  &grid_info, sizeof(grid_info), 0);
	}

	vol->bake_shader_handle = A_Require(assets,
										  String8Lit("assets://shaders/passes/ibl/irradiance_probe_bake.slang"),
										  A_Type_Shader);

	vol->is_baked = false;

	DebugLogI(log_channel,
			  "Irradiance Volume Initialized (%u * %u * %u = %u probes).",
			  nx, ny, nz, vol->ntotal);
}


internal void
R_IrradianceVolumeDestroy(R_IrradianceVolume *vol)
{
	if (vol->blas_count > 0)
	{
		for (u32 i = 0; i < vol->blas_count; i++)
			G_DeviceAccelStructDestroy(vol->device, vol->blas_per_page[i]);
	}

	if (vol->is_baked)
	{
		G_DeviceAccelStructDestroy(vol->device, vol->tlas);
	}

	G_DeviceBufferDestroy(vol->device, vol->sh_buffer);
	G_DeviceBufferDestroy(vol->device, vol->grid_info_buffer);
}

internal void
R_IrradianceVolumeBuildAccelStructs(R_IrradianceVolume *vol, const R_Scene *scene)
{
	G_Device *device = vol->device;

	u64 max_scratch_size = 0;

	vol->blas_count = scene->geometry_page_count;
	AssertTrue(vol->blas_count < ArraySize(vol->blas_per_page));

	for (u32 page_index = 0; page_index < scene->geometry_page_count; page_index++)
	{
		const R_GeometryPage *page = &scene->geometry_pages[page_index];
		
		G_BLASGeometry geometry = {0};
		
		geometry.vertex_buffer = page->vertex_buffer;
		geometry.vertex_count  = page->vertex_count;
		geometry.vertex_stride = sizeof(R_GPU_ModelVertex);
		geometry.vertex_format = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.vertex_offset = 0;

		geometry.index_buffer  = page->index_buffer;
		geometry.index_count   = page->index_count;
		geometry.index_type    = VK_INDEX_TYPE_UINT32;
		geometry.index_offset  = 0;

		G_DeviceAllocAccelStructReceipt receipt = G_DeviceBLASAlloc(device, &geometry, 1);
		vol->blas_per_page[page_index] = receipt.key;
		max_scratch_size = MaxValue(max_scratch_size, receipt.scratch_size);
	}
	
	G_DeviceAllocAccelStructReceipt tlas_receipt = G_DeviceTLASAlloc(device, scene->object_count);
	vol->tlas = tlas_receipt.key;
	max_scratch_size = MaxValue(max_scratch_size, tlas_receipt.scratch_size);

	G_BufferAllocInfo scratch_info = {0};
	scratch_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	scratch_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	scratch_info.size  = max_scratch_size;

	G_BufferKey scratch_buffer = G_DeviceBufferAlloc(device, &scratch_info);

	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkAccelerationStructureInstanceKHR *instances = ArenaPushArray(scratch.arena, VkAccelerationStructureInstanceKHR, scene->object_count);

	u32 instance_index = 0;

	for (u32 i = 0; i < R_SCENE_MAX_OBJECTS && instance_index < scene->object_count; i++)
	{
		const R_ObjectSlot *slot = &scene->object_slots[i];

		if (!slot->active)
			continue;

		m4 t = slot->transform;
		
		VkAccelerationStructureInstanceKHR *inst = &instances[instance_index];
		
		inst->transform.matrix[0][0] = t.m00;
		inst->transform.matrix[0][1] = t.m01;
		inst->transform.matrix[0][2] = t.m02;
		inst->transform.matrix[0][3] = t.m03;
		inst->transform.matrix[1][0] = t.m10;
		inst->transform.matrix[1][1] = t.m11;
		inst->transform.matrix[1][2] = t.m12;
		inst->transform.matrix[1][3] = t.m13;
		inst->transform.matrix[2][0] = t.m20;
		inst->transform.matrix[2][1] = t.m21;
		inst->transform.matrix[2][2] = t.m22;
		inst->transform.matrix[2][3] = t.m23;

		inst->instanceCustomIndex = i;
		inst->mask = 0xFF;
		inst->instanceShaderBindingTableRecordOffset = 0;
		inst->flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

		// TODO: this is ASS
		u32 mesh_index = slot->mesh.index;
		u32 page_idx = scene->mesh_slots[mesh_index].page_index;
		inst->accelerationStructureReference = G_DeviceAccelStructAddress(device, vol->blas_per_page[page_idx]);

		instance_index++;
	}

	u64 instance_data_size = scene->object_count * sizeof(VkAccelerationStructureInstanceKHR);

	G_BufferAllocInfo inst_buf_info = {0};
	inst_buf_info.usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
	inst_buf_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // MUST HAVE DEVICEADDRESS ALIGNED TO 16 BYTES SO ALLOCATE DEDICATED NEW MEMORY
	inst_buf_info.size = instance_data_size;

	G_BufferKey instance_buffer = G_DeviceBufferAlloc(device, &inst_buf_info);
	G_DeviceBufferWrite(device, instance_buffer, instances, instance_data_size, 0);

	G_CmdBuffer cmd = G_DeviceSubmitImBegin(device);
	{
		for (u32 page_index = 0; page_index < scene->geometry_page_count; page_index++)
		{
			const R_GeometryPage *page = &scene->geometry_pages[page_index];
		
			G_BLASGeometry geometry = {0};

			geometry.vertex_buffer = page->vertex_buffer;
			geometry.vertex_count  = page->vertex_count;
			geometry.vertex_stride = sizeof(R_GPU_ModelVertex);
			geometry.vertex_format = VK_FORMAT_R32G32B32_SFLOAT;
			geometry.vertex_offset = 0;
			
			geometry.index_buffer  = page->index_buffer;
			geometry.index_count   = page->index_count;
			geometry.index_type    = VK_INDEX_TYPE_UINT32;
			geometry.index_offset  = 0;

			G_CmdBuildBLAS(&cmd, vol->blas_per_page[page_index], &geometry, 1, scratch_buffer);
		}
	
		// BLAS -> TLAS.
		{
			G_AccessSt access_src = { VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR };
			G_AccessSt access_dst = { VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR };

			VkMemoryBarrier2 barrier = G_SyncMemoryBarrier(&access_src, &access_dst);
			
			G_CmdPipelineBarrier(&cmd, 0, 1, &barrier, 0, NULL, 0, NULL);
		}

		G_CmdBuildTLAS(&cmd, vol->tlas, instance_buffer, scene->object_count, scratch_buffer);

		// TLAS -> Compute Shader Ray Query.
		{
			G_AccessSt access_src = { VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR };
			G_AccessSt access_dst = { VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,                   VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR };

			VkMemoryBarrier2 barrier = G_SyncMemoryBarrier(&access_src, &access_dst);
			
			G_CmdPipelineBarrier(&cmd, 0, 1, &barrier, 0, NULL, 0, NULL);
		}
	}
	G_DeviceSubmitImEnd(device, &cmd);

	G_DeviceBufferDestroy(device, scratch_buffer);
	G_DeviceBufferDestroy(device, instance_buffer);

	ScratchRelease(&scratch);

	DebugLogI(vol->log_channel,
			  "Built acceleration structures (%u BLAS, %u instances).",
			  vol->blas_count, scene->object_count);
}

internal void
R_IrradianceVolumeBake(R_IrradianceVolume *vol, const R_Scene *scene)
{
	G_Device *device = vol->device;

	DebugLogI(vol->log_channel, "Baking irradiance volume...");

	R_IrradianceVolumeBuildAccelStructs(vol, scene);

	G_ShaderKey shader = A_GetNow(vol->assets, vol->bake_shader_handle, A_Type_Shader)->shader.key;

	G_ComputePipelineDef pipeline_def = G_ComputePipelineDefInit(shader);
	G_PipelineSt pipeline_st = G_DeviceFetchComputePipeline(device, &pipeline_def);

	struct
	{
		u64 sh_buffer;        // Out: <R_GPU_ProbeSH[]>
		u64 grid_info_buffer; // In:  <R_GPU_ProbeGridInfo>
		u64 tlas_address;

		u32 environment_cubemap;
		u32 linear_sampler;

		u32 rays_per_probe;
		u32 _padding;
	}
	pc;

	pc.sh_buffer           = G_DeviceBufferAddress       (device, vol->sh_buffer);
	pc.grid_info_buffer    = G_DeviceBufferAddress       (device, vol->grid_info_buffer);
	pc.tlas_address        = G_DeviceAccelStructAddress  (device, vol->tlas);
	pc.environment_cubemap = G_DeviceTextureViewBindless (device, vol->environment_view);
	pc.linear_sampler      = G_DeviceSamplerBindless     (device, vol->linear_sampler);
	pc.rays_per_probe      = R_IRRADIANCE_RAYS_PER_PROBE;

	G_CmdBuffer cmd = G_DeviceSubmitImBegin(device);
	{
		G_CmdBindBindless  (&cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
		G_CmdBindPipeline  (&cmd, pipeline_st.bind_point, pipeline_st.pipeline);
		G_CmdPushConstants (&cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(pc), &pc, 0);
		G_CmdDispatch      (&cmd, G_ComputeGroupCount(vol->ntotal, 64), 1, 1);
	}
	G_DeviceSubmitImEnd(device, &cmd);

	vol->is_baked = true;

	DebugLogI(vol->log_channel, "Irradiance volume baked (%u probes, %u rays each).",
			  vol->ntotal, R_IRRADIANCE_RAYS_PER_PROBE);
}

internal void
R_IrradianceVolumeDebug(const R_IrradianceVolume *vol)
{
	u32 nx = vol->nx;
	u32 ny = vol->ny;
	u32 nz = vol->nz;
	
	v3 cell_size = v3((nx > 1) ? (vol->grid_max.x - vol->grid_min.x) / (f32)(nx - 1) : 0.f,
					  (ny > 1) ? (vol->grid_max.y - vol->grid_min.y) / (f32)(ny - 1) : 0.f,
					  (nz > 1) ? (vol->grid_max.z - vol->grid_min.z) / (f32)(nz - 1) : 0.f);
	
	for (u32 x = 0; x < nx; x++)
	{
		for (u32 y = 0; y < ny; y++)
		{
			for (u32 z = 0; z < nz; z++)
			{
				v3 position = vol->grid_min;

				position = V3Add(position, V3MulV3(cell_size, v3(x, y, z)));


				// TODO: actually render an opaque sphere with the irradiance
				//       shown on it for actually good debugging and not this
				//       pathetic excuse.
				
				R_DebugPushSphere(position, 0.1f,
								  v4(1.f, 1.f, 1.f, 1.f),
								  0.f, true);
		}
	}
}
}

internal G_BufferKey
R_IrradianceVolumeGetSHBuffer(const R_IrradianceVolume *vol)
{
	return vol->sh_buffer;
}

internal G_BufferKey
R_IrradianceVolumeGetGridInfoBuffer(const R_IrradianceVolume *vol)
{
	return vol->grid_info_buffer;
}

internal b32
R_IrradianceVolumeIsBaked(const R_IrradianceVolume *vol)
{
	return vol->is_baked;
}

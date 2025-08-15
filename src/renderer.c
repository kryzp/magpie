

typedef struct MyVertex
{
	v3 position;
	v2 texcoord;
}
MyVertex;

internal void
RendererInit(Renderer *renderer, MemoryArena *arena) // TODO(kp): Arena is temporarily passed in here to do loading in of shaders.
{
	// NOTE(kp): Vertex Format.
	{
		AddVertexBinding(&renderer->my_vertex_format, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);
		{
			AddVertexAttribute(&renderer->my_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, position));
			AddVertexAttribute(&renderer->my_vertex_format, VK_FORMAT_R32G32_SFLOAT,    offsetof(MyVertex, texcoord));
		}
	}
	
	// NOTE(kp): Mesh.
	{
		MyVertex vertices[] = {
			{ { 0.f, 0.f, -5.f }, { 0.f, 1.f } },
			{ { 0.f, 1.f, -5.f }, { 0.f, 0.f } },
			{ { 1.f, 0.f, -5.f }, { 1.f, 1.f } }
		};
		
		u16 indices[] = {
			0, 1, 2
		};
		
		u64 vertex_buffer_size = sizeof(vertices);
		u64 index_buffer_size = sizeof(indices);
		
		renderer->my_vertex_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
													   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
													   vertex_buffer_size);
		
		renderer->my_index_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
													  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
													  index_buffer_size);
		
		GPUBuffer staging_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
													 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
													 vertex_buffer_size + index_buffer_size);
		{
			GPUBufferWrite(&staging_buffer, vertices, vertex_buffer_size, 0);
			GPUBufferWrite(&staging_buffer, indices, index_buffer_size, vertex_buffer_size);
			
			CommandBuffer cmd = BeginGraphicsInstantSubmit();
			{
				VkBufferCopy stage_to_vertex_copy = {0};
				stage_to_vertex_copy.srcOffset = 0;
				stage_to_vertex_copy.dstOffset = 0;
				stage_to_vertex_copy.size = vertex_buffer_size;
				
				CmdCopyBufferToBuffer(&cmd,
									  &staging_buffer,
									  &renderer->my_vertex_buffer,
									  1, &stage_to_vertex_copy);
				
				VkBufferCopy stage_to_index_copy = {0};
				stage_to_index_copy.srcOffset = vertex_buffer_size;
				stage_to_index_copy.dstOffset = 0;
				stage_to_index_copy.size = index_buffer_size;
				
				CmdCopyBufferToBuffer(&cmd,
									  &staging_buffer,
									  &renderer->my_index_buffer,
									  1, &stage_to_index_copy);
			}
			EndGraphicsInstantSubmit(&cmd);
		}
		GraphicsWaitIdle();
		GPUBufferDestroy(&staging_buffer);
	}
	
	// NOTE(kp): Shader.
	{
		renderer->my_shader_program.push_constant_size = sizeof(m4) + sizeof(u32);
		renderer->my_shader_program.layout_count = 1;
		renderer->my_shader_program.layouts[0] = graphics_device->bindless.bindless_layout;
		
		renderer->my_shader_program.stage_count = 2;
		renderer->my_shader_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->my_shader_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	// NOTE(kp): Graphics Pipeline.
	{
		GraphicsPipelineDef definition = {0};
		definition.program = &renderer->my_shader_program;
		definition.vertex_format = &renderer->my_vertex_format;
		definition.cull_mode = VK_CULL_MODE_BACK_BIT;
		definition.front_face = VK_FRONT_FACE_CLOCKWISE;
		definition.blend_state = BlendStateDefault();
		definition.depth_stencil_state = DepthStencilStateDefault();
		definition.depth_stencil_state.depth_test_enabled = 0;
		definition.depth_stencil_state.depth_write_enabled = 0;
		definition.colour_attachment_count = 1;
		definition.colour_attachment_formats[0] = graphics_device->swapchain.format;
		definition.has_depth_attachment = 0;
		definition.min_sample_shading_enabled = 1;
		definition.min_sample_shading = 0.2f;
		definition.samples = VK_SAMPLE_COUNT_1_BIT;
		
		renderer->my_pipeline_layout = PipelineLayoutCreate(&renderer->my_shader_program);
		renderer->my_pipeline = GraphicsPipelineCreate(renderer->my_pipeline_layout, &definition);
	}
	
	// NOTE(kp): Combined Image Sampler.
	{
		renderer->my_image = ImageFromPath(str8("res/image.png"));
		renderer->my_image_view = GetStandardImageView(&renderer->my_image);
		
		renderer->my_sampler = SamplerInitFilter(VK_FILTER_NEAREST);
		
		BindlessResourcesBindCombinedImage(&graphics_device->bindless,
										   0, &renderer->my_image_view, &renderer->my_sampler);
	}
}

internal void
RendererDestroy(Renderer *renderer)
{
	ImageViewDestroy(&renderer->my_image_view);
	ImageDestroy(&renderer->my_image);
	SamplerDestroy(&renderer->my_sampler);
	
	GPUBufferDestroy(&renderer->my_vertex_buffer);
	GPUBufferDestroy(&renderer->my_index_buffer);
	
	ShaderStageDestroy(&renderer->my_shader_program.stages[0]);
	ShaderStageDestroy(&renderer->my_shader_program.stages[1]);
	
	PipelineLayoutDestroy(renderer->my_pipeline_layout);
	PipelineDestroy(renderer->my_pipeline);
}

internal void
RendererBeginFrame(Renderer *renderer)
{
	renderer->present_cmd = BeginGraphicsPresent();
	
	RenderInfo render_info = {0};
	render_info.width = graphics_device->swapchain.width;
	render_info.height = graphics_device->swapchain.height;
	render_info.samples = VK_SAMPLE_COUNT_1_BIT;
	
	RenderInfoAddColourAttachment(&render_info,
								  VK_ATTACHMENT_LOAD_OP_CLEAR,
								  GetCurrentSwapchainImageView(&graphics_device->swapchain),
								  0,
								  v4(1.f, 0.f, 0.f, 1.f));
	
	CmdTransitionImageLayout(&renderer->present_cmd,
							 GetCurrentSwapchainImage(&graphics_device->swapchain),
							 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	
	CmdBeginRendering(&renderer->present_cmd, &render_info);
	{
		// 1. Bind the pipeline.
		// 2. Bind the descriptor set.
		// 3. Bind the vertex buffer.
		// 4. Bind the index buffer.
		// 5. Push constants.
		// 6. Draw indexed.
		
		struct
		{
			m4 transform_matrix;
			u32 texture_id;
		}
		args;
		
		static f32 x = 0.f;
		x += 0.02f;
		
		args.transform_matrix = m4(1.f);
		args.transform_matrix = M4MultiplyM4(M4TranslateV3(v3(x, 0.f, 0.f)), args.transform_matrix);
		args.transform_matrix = M4MultiplyM4(M4Perspective(70.f, 1280.f/720.f, 0.1f, 10.f), args.transform_matrix);
		
		args.texture_id = 0;
		
		CmdBindDescriptors(&renderer->present_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->my_pipeline_layout, 0, 1, &graphics_device->bindless.bindless_set, 0, 0);
		CmdBindPipeline(&renderer->present_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->my_pipeline);
		CmdBindVertexBuffer(&renderer->present_cmd, renderer->my_vertex_format.bindings[0].binding, &renderer->my_vertex_buffer, 0);
		CmdBindIndexBuffer(&renderer->present_cmd, &renderer->my_index_buffer, 0);
		CmdPushConstants(&renderer->present_cmd, renderer->my_pipeline_layout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(args), &args);
		CmdDrawIndexed(&renderer->present_cmd, 3, 1, 0, 0, 0);
	}
	CmdEndRendering(&renderer->present_cmd);
	
	/*
	RenderPassDef pass = {0};
// ...

	RendererPushPass(&renderer, &pass);
*/
}

internal void
RendererEndFrame(Renderer *renderer)
{
	//RendererExportToCommandBuffer(&renderer, renderer->present_cmd);
	
	EndGraphicsPresent(&renderer->present_cmd);
}

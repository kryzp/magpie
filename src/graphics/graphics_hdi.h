#ifndef GRAPHICS_HDI_H
#define GRAPHICS_HDI_H

// backend api should look something like this (eventually (????)) ?
// not sure what to do about stuff like pipeline barriers, synchronisation, etc...

typedef struct G_HDI G_HDI;
struct G_HDI
{
	void *(*InitAndSelect)(Arena *arena, LOG_Channel log_channel);
	void (*Destroy)(void);
	void (*Select)(void *ctx);

	VkFormat (*GetDepthFormat)(void);
	u32 (*GetFrameInFlightIndex)(void);

	b32 (*IsFeatureEnabled)(G_FeatureType type);

	G_Handle (*BeginFrame)(G_Handle swapchain);
	void (*EndFrame)(G_Handle swapchain, G_Handle cmd);

	G_TimelinePoint (*Submit)(G_Handle cmd);
	G_TimelinePoint (*SubmitEx)(G_Handle cmd, ...);

	G_Handle (*SubmitImBegin)(void);
	void (*SubmitImEnd)(G_Handle cmd);

	void (*HotLoad)(void);
	void (*HotUnload)(void);

	void (*WaitIdle)(void);
	void (*WaitForFence)(G_Handle fence);
	void (*ResetFence)(G_Handle fence);
	void (*DestroyFence)(G_Handle fence);
	G_Handle (*SemaphoreCreate)(u64 value);
	void (*SemaphoreDestroy)(G_Handle semaphore);
	u64 (*GetSemaphoreGpuCounterValue)(G_Handle semaphore);
	void (*WaitUntil)(G_TimelinePoint point);

	G_Handle (*SwapchainCreate)(void);
	void (*SwapchainDestroy)(void);

	G_PipelineSt (*FetchGraphicsPipeline)(const G_GraphicsPipelineDef *def);
	G_PipelineSt (*FetchComputePipeline)(const G_ComputePipelineDef *def);

	G_Handle (*TextureAlloc)(const G_TextureAllocInfo *alloc_info);
	void (*TextureDestroy)(G_Handle texture);
	G_TextureInfo (*TextureGetInfo)(G_Handle texture);

	G_Handle (*TextureViewFetch)(const G_TextureViewCreateInfo *info);
	G_Handle (*TextureViewAuto)(G_Handle texture);
	u32 (*TextureViewBindless)(G_Handle view);

	G_Handle (*BufferAlloc)(const G_BufferAllocInfo *info);
	void (*BufferDestroy)(G_Handle buffer);
	void *(*BufferMap)(G_Handle buffer);
	u64 (*BufferAddress)(G_Handle handle);
	void (*BufferRead)(G_Handle buffer, void *dst, u64 length, u64 offset);
	void (*BufferWrite)(G_Handle buffer, const void *src, u64 length, u64 offset);
	G_BufferInfo (*BufferGetInfo)(G_Handle buffer);

	G_Handle (*SamplerCreate)(const G_SamplerCreateInfo *info);
	void (*SamplerDestroy)(G_Handle sampler);
	u32 (*SamplerBindless)(G_Handle sampler);
	G_SamplerInfo (*SamplerGetInfo)(G_Handle sampler);

	G_Handle (*ProgramCreate)(u32 stage_count, const G_ShaderBytecode *stages);
	void (*ProgramDestroy)(G_Handle program);

	G_AccelStructReceipt (*BlasAlloc)(const G_BlasGeom *geometries, u32 geometry_count);
	G_AccelStructReceipt (*TlasAlloc)(u32 max_instance_count);
	void (*AccelStructDestroy)(G_Handle as);
	u64 (*AccelStructAddress)(G_Handle as);
	
	void (*ImGuiNewFrame)(void);
	void (*ImGuiRecordInto)(G_Handle cmd);

	void (*CmdBegin)(G_Handle cmd);
	void (*CmdEnd)(G_Handle cmd);
	void (*CmdBeginRendering)(G_Handle cmd, const G_RenderInfo *info);
	void (*CmdEndRendering)(G_Handle cmd);
	void (*CmdSetViewport)(G_Handle cmd, VkViewport viewport);
	void (*CmdSetScissor)(G_Handle cmd, VkRect2D scissor);
	
	void (*CmdPipelineBarrier)(G_Handle cmd,
							   VkDependencyFlags dependency_flags,
							   u32 memory_barrier_count, const VkMemoryBarrier2 *memory_barriers,
							   u32 buffer_barrier_count, const VkBufferMemoryBarrier2 *buffer_barriers,
							   u32 image_barrier_count, const VkImageMemoryBarrier2 *image_barriers);

	void (*CmdBindDescriptors)(G_Handle cmd,
							   VkShaderStageFlags stage_flags,
							   G_Handle layout, u32 first,
							   u32 descriptor_count, const VkDescriptorSet *descriptors,
							   u32 dynamic_offset_count, const u32 *dynamic_offsets);

	void (*CmdBindBindless)(G_Handle cmd,
							VkShaderStageFlags stage_flags,
							G_Handle layout);

	void (*CmdBindPipeline)(G_Handle cmd,
							VkPipelineBindPoint bind_point,
							G_Handle pipeline);

	void (*CmdBindIndexBuffer)(G_Handle cmd,
							   G_Handle buffer,
							   u64 offset, u64 size, // VK_WHOLE_SIZE
							   VkIndexType type);

	void (*CmdPushConstantsEx)(G_Handle cmd,
							   G_Handle layout,
							   VkShaderStageFlags stage_flags,
							   u64 size, const void *data,
							   u32 offset);

	void (*CmdSetLineWidth)(G_Handle cmd, f32 thickness);

	void (*CmdDraw)(G_Handle cmd,
					u32 vertex_count,
					u32 instance_count,
					u32 first_vertex,
					u32 first_instance);

	void (*CmdDrawIndexed)(G_Handle cmd,
						   u32 index_count,
						   u32 instance_count,
						   u32 first_index,
						   i32 vertex_offset,
						   u32 first_instance);

	void (*CmdDrawIndexedIndirect)(G_Handle cmd,
								   G_Handle buffer, u64 offset,
								   u32 count, u32 stride);

	void (*CmdDrawIndexedIndirectCount)(G_Handle cmd,
										G_Handle indirect_buffer, u64 indirect_offset,
										G_Handle count_buffer, u64 count_offset,
										u32 max_count, u32 stride);

	void (*CmdDrawMeshTasksIndirectCount)(G_Handle cmd,
										  G_Handle indirect_buffer, u64 indirect_offset,
										  G_Handle count_buffer,u64 count_offset,
										  u32 max_count, u32 stride);

	void (*CmdDispatch)(G_Handle cmd, u32 x, u32 y, u32 z);

	void (*CmdDispatchIndirect)(G_Handle cmd, G_Handle buffer, u64 offset);

	void (*CmdBlit)(G_Handle cmd,
					G_Handle src_texture,
					G_Handle dst_texture,
					u32 region_count, const VkImageBlit2 *regions,
					VkFilter filter);

	void (*CmdGenerateMipmaps)(G_Handle cmd, G_Handle texture);

	void (*CmdCopyBufferToBuffer)(G_Handle cmd,
								  G_Handle src_buffer,
								  G_Handle dst_buffer,
								  u32 region_count, const G_BufferCopy *regions);

	void (*CmdCopyBufferToTexture)(G_Handle cmd,
								   G_Handle src_buffer,
								   G_Handle dst_texture,
								   u32 region_count, const G_BufferImageCopy *regions);

	void (*CmdCopyBufferToTextureWhole)(G_Handle cmd,
										G_Handle src_buffer,
										G_Handle dst_texture,
										u64 buffer_offset);

	void (*CmdFillBuffer)(G_Handle cmd,
						  G_Handle buffer,
						  u64 offset, u64 size, u32 fill);

	void (*CmdBuildBlas)(G_Handle cmd,
						 G_Handle blas,
						 const G_BLASGeometry *geometries, u32 geometry_count,
						 G_Handle scratch_buffer);

	void (*CmdBuildTlas)(G_Handle cmd,
						 G_Handle tlas,
						 G_Handle instance_buffer, u32 instance_count,
						 G_Handle scratch_buffer);

	void (*CmdBeginQuery)(G_Handle cmd,
						  VkQueryPool pool,
						  u32 query, VkQueryControlFlags flags);

	void (*CmdEndQuery)(G_Handle cmd,
						VkQueryPool pool,
						u32 query);

	void (*CmdResetQueries)(G_Handle cmd,
							VkQueryPool pool,
							u32 first, u32 count);

	void (*CmdWriteTimestamp)(G_Handle cmd,
							  VkPipelineStageFlags2 stage,
							  VkQueryPool pool,
							  u32 index);

	void (*CmdBeginLabel)(G_Handle cmd, String8 name);
	void (*CmdBeginLabelEx)(G_Handle cmd, String8 name, v4 colour);
	void (*CmdEndLabel)(G_Handle cmd);
};

static G_HDI *ghdi = NULL;

#endif // GRAPHICS_HDI_H

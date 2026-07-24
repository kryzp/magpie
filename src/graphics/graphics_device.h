#ifndef GRAPHICS_DEVICE_H
#define GRAPHICS_DEVICE_H

// TODO: DEVICE DESTROY() FUNCTIONS FOR RESOURCES CURRENTLY DON'T FREE FROM
//       THE RESOURCE LIST - FIX!

/* ==================================================
   ALLOC / CREATION PARAMETERS
   ================================================== */

typedef u32 G_TextureAllocFlags;
enum
{
	G_TextureAllocFlag_None      = 0,
	G_TextureAllocFlag_Transient = 1 << 0,
	G_TextureAllocFlag_Storage   = 1 << 1,
	G_TextureAllocFlag_Cubemap   = 1 << 2
};

typedef struct G_TextureAllocInfo G_TextureAllocInfo;
struct G_TextureAllocInfo
{
	u32 width;
	u32 height;
	u32 depth;

	VkFormat format;
	VkImageType type;
	VkImageTiling tiling;

	u32 mipmaps;
	u32 layers;

	VkSampleCountFlagBits samples;

	G_TextureAllocFlags flags;
};

typedef struct G_TextureViewCreateInfo G_TextureViewCreateInfo;
struct G_TextureViewCreateInfo
{	
	G_ResourceKey texture;
	VkImageViewType type;
	G_SubresourceRange range;
};

typedef struct G_BufferAllocInfo G_BufferAllocInfo;
struct G_BufferAllocInfo
{
	VkBufferUsageFlags2 usage;
	VmaAllocationCreateFlags flags;
	u64 size;
};

typedef struct G_SamplerCreateInfo G_SamplerCreateInfo;
struct G_SamplerCreateInfo
{
	VkFilter filter;
	VkSamplerAddressMode wrap_x;
	VkSamplerAddressMode wrap_y;
	VkSamplerAddressMode wrap_z;
	VkBorderColor border_colour;
};


/* ==================================================
   DEVICE
   ================================================== */

typedef struct G_SwapchainSupportDetails G_SwapchainSupportDetails;
struct G_SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	u32 surface_format_count;
	VkSurfaceFormatKHR *surface_formats;

	u32 present_mode_count;
	VkPresentModeKHR *present_modes;
};

typedef struct G_HardwareQueue G_HardwareQueue;
struct G_HardwareQueue
{
	VkQueue vk_handle;
	u32 family_index;
};

typedef struct G_DestroyedImage G_DestroyedImage;
struct G_DestroyedImage
{
	G_DestroyedImage *next;
	G_ResourceKey key;
	VkImage image;
	VmaAllocation allocation;
};

typedef struct G_DestroyedBuffer G_DestroyedBuffer;
struct G_DestroyedBuffer
{
	G_DestroyedBuffer *next;
	G_ResourceKey key;
	VkBuffer buffer;
	VmaAllocation allocation;
};

typedef struct G_DestroyedSampler G_DestroyedSampler;
struct G_DestroyedSampler
{
	G_DestroyedSampler *next;
	G_ResourceKey key;
	VkSampler sampler;
	u32 bindless;
};

typedef struct G_DestroyedShaderProgram G_DestroyedShaderProgram;
struct G_DestroyedShaderProgram
{
	G_DestroyedShaderProgram *next;
	G_ResourceKey key;
};

typedef struct G_DestroyedAccelStruct G_DestroyedAccelStruct;
struct G_DestroyedAccelStruct
{
	G_DestroyedAccelStruct *next;
	G_ResourceKey key;
	VkAccelerationStructureKHR handle;
};

typedef struct G_FrameInFlight G_FrameInFlight;
struct G_FrameInFlight
{
	Arena arena;
	
	G_TimelinePoint completion_point;

	G_CmdPool command_pool;

	VkSemaphore image_available_semaphore; // Wait until the OS gives us an image.
	
	G_DestroyedImage           *destroyed_image_head;
	G_DestroyedBuffer          *destroyed_buffer_head;
	G_DestroyedSampler         *destroyed_sampler_head;
	G_DestroyedShaderProgram   *destroyed_shader_head;
	G_DestroyedAccelStruct     *destroyed_as_head;
};

// todo: make some kinda limits struct / expose ?

typedef struct G_Device G_Device;
struct G_Device
{
	Arena *permanent_arena;

	LOG_Channel log_channel;	
	LOG_Channel log_channel_general;
	LOG_Channel log_channel_validation;
	LOG_Channel log_channel_performance;
	LOG_Channel log_channel_cmd_buffer;
	
	VkInstance vk_instance;
	VkDevice vk_device;

	VkPhysicalDevice vk_physical_device;
	VkPhysicalDeviceProperties2 vk_physical_device_properties;
	VkPhysicalDeviceFeatures2 vk_physical_device_features;
	
	VkSurfaceKHR vk_surface;

	VmaAllocator vma_allocator;

	G_HardwareQueue graphics_queue;

	VkPipelineCache pipeline_process_cache;

	b32 has_validation_layers;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkFormat depth_format;
	VkSampleCountFlagBits max_msaa_samples;
	u64 max_push_constants_size; // todo: move into a seperate limits struct

	G_ResolvedFeatures features;
	
	G_SwapchainSupportDetails swapchain_details;

	u32 current_frame_in_flight_index;
	G_FrameInFlight frames_in_flight[G_FRAMES_IN_FLIGHT];

	G_ResourceList pipeline_layouts;
	G_ResourceList pipelines;
	G_ResourceList textures;
	G_ResourceList texture_views;
	G_ResourceList buffers;
	G_ResourceList samplers;
	G_ResourceList shaders;
	G_ResourceList accel_structures;
	
	G_Semaphore graphics_semaphore;

	G_Bindless bindless;
	
	VkDescriptorPool imgui_pool;
};


/* ==================================================
   INTERNALS
   ================================================== */

internal VkFormat G_FindGraphicsSupportedFormat(VkPhysicalDevice physical_device,
												VkImageTiling tiling,
												VkFormatFeatureFlags features,
												u32 candidate_count, const VkFormat *candidates,
												LOG_Channel log_channel);

internal VkFormat G_FindGraphicsDepthFormat(VkPhysicalDevice physical_device,
											LOG_Channel log_channel);

internal VkSampleCountFlagBits G_FindGraphicsMaxUsableSampleCount(VkPhysicalDeviceProperties2 properties,
																  LOG_Channel log_channel);

internal const char * const *G_GetInstanceExtensions(Arena *arena,
													 u32 *extension_count,
													 LOG_Channel log_channel);

internal b32 G_CheckForValidationLayerSupport(void);

internal G_SwapchainSupportDetails G_QuerySwapchainSupport(Arena *arena,
														   VkPhysicalDevice physical_device,
														   VkSurfaceKHR surface);

internal VKAPI_ATTR VkBool32 VKAPI_CALL G_VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
															  VkDebugUtilsMessageTypeFlagsEXT message_type,
															  const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
															  void *user_data);

internal VkResult G_CreateDeviceDebugUtilsMessengerExt(VkInstance instance,
													   VkDebugUtilsMessengerCreateInfoEXT *debug_info,
													   const VkAllocationCallbacks *allocator,
													   VkDebugUtilsMessengerEXT *messenger);

internal VkSurfaceFormatKHR G_ChooseSwapchainSurfaceFormat(LOG_Channel channel,
														   u32 available_surface_format_count,
														   const VkSurfaceFormatKHR *available_surface_formats);

internal VkPresentModeKHR G_ChooseSwapchainPresentMode(u32 available_present_mode_count,
													   const VkPresentModeKHR *available_present_modes,
													   b32 enable_vsync);

internal VkExtent2D G_ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR *capabilities);


/* ==================================================
   CORE DEVICE
   ================================================== */

typedef struct G_FeatureRequest
{
	G_FeatureType type;
	G_FeatureTier tier;
}
G_FeatureRequest;

internal void G_InitAndSelect(G_Device *device, Arena *arena, LOG_Channel log_channel,
							  G_FeatureRequest *requested_features, u32 feature_count);

internal void G_Destroy(void);

internal void G_SelectContext(G_Device *device);
internal G_Device *G_GetSelected(void);

internal VkFormat G_GetDepthFormat(void);
internal u32 G_GetFrameInFlightIndex(void);

internal b32 G_FeatureEnabled(G_FeatureType type);

internal void G_FlushInFlightFrame(G_FrameInFlight *frame);

internal G_CmdBuffer G_BeginFrame(G_Swapchain *swapchain);
internal void G_EndFrame(const G_Swapchain *swapchain, const G_CmdBuffer *cmd);

internal G_TimelinePoint G_Submit(const G_CmdBuffer *cmd);

internal G_TimelinePoint G_SubmitEx(const G_CmdBuffer *cmd,
									u32 wait_count, const VkSemaphoreSubmitInfo *waits,
									u32 signal_count, const VkSemaphoreSubmitInfo *signals);

internal G_CmdBuffer G_SubmitImBegin(void);
internal void  G_SubmitImEnd(const G_CmdBuffer *cmd);

internal void G_HotLoad(void);
internal void G_HotUnload(void);

internal void G_CreateSyncResources(void);
internal void G_DestroySyncResources(void);

internal void G_CreateBindless(void);
internal void G_ApplyBindlessUpdates(void);
internal void G_DestroyBindless(void);

internal void G_CreateImGui(void);
internal void G_DestroyImGui(void);


/* ==================================================
   QUERY
   ================================================== */

internal VkQueryPool G_QueryPoolCreate(u32 query_count, VkQueryType type, VkQueryPipelineStatisticFlags pipeline_stat_flags);
internal void G_QueryPoolDestroy(VkQueryPool pool);


/* ==================================================
   SYNCHRONISATION
   ================================================== */

internal void G_WaitIdle(void);

internal void G_WaitForFence(VkFence fence);
internal void G_ResetFence(VkFence fence);
internal void G_DestroyFence(VkFence fence);

internal G_Semaphore G_SemaphoreCreate(u64 value);
internal void G_SemaphoreDestroy(const G_Semaphore *semaphore);
internal u64 G_SemaphoreGPUCounterValue(const G_Semaphore *semaphore);

internal void G_WaitUntil(G_TimelinePoint point);


/* ==================================================
   SWAPCHAIN
   ================================================== */

internal G_Swapchain G_SwapchainCreate(void);
internal void G_SwapchainDestroy(const G_Swapchain *swapchain);


/* ==================================================
   COMMAND POOL
   ================================================== */

internal G_CmdPool G_CmdPoolCreate(u32 family_index);
internal void G_CmdPoolDestroy(const G_CmdPool *pool);

internal G_CmdBuffer G_CmdPoolAcquire(G_CmdPool *pool);
internal void G_CmdPoolRelease(G_CmdPool *pool, const G_CmdBuffer *cmd, u64 fence_value);
internal void G_CmdPoolPurge(G_CmdPool *pool, u64 fence_value);


/* ==================================================
   PIPELINES
   ================================================== */

internal G_ResourceKey G_PipelineLayoutFetch(G_ResourceKey program);
internal VkPipelineLayout G_PipelineLayoutFromKey(G_ResourceKey key);


typedef struct G_PipelineSt G_PipelineSt;
struct G_PipelineSt
{
	G_ResourceKey pipeline;
	G_ResourceKey layout;
	VkPipelineBindPoint bind_point;
};

internal G_PipelineSt G_FetchGraphicsPipeline(const G_GraphicsPipelineDef *def);
internal G_PipelineSt G_FetchComputePipeline(const G_ComputePipelineDef *def);

internal VkPipeline G_PipelineFromKey(G_ResourceKey key);


/* ==================================================
   TEXTURES
   ================================================== */

internal G_ResourceKey G_TextureAlloc(const G_TextureAllocInfo *alloc_info);
internal G_ResourceKey G_TextureAlloc2D(u32 width, u32 height, VkFormat format, u32 mipmaps);
internal G_ResourceKey G_TextureAlloc2DRW(u32 width, u32 height, VkFormat format, u32 mipmaps);
internal G_ResourceKey G_TextureAllocDepth2D(u32 width, u32 height, u32 mipmaps);
internal G_ResourceKey G_TextureAllocDepth2DRW(u32 width, u32 height, u32 mipmaps);
internal G_ResourceKey G_TextureAllocCubemap(u32 resolution, VkFormat format, u32 mipmaps);
internal G_ResourceKey G_TextureAllocCubemapDepth(u32 resolution, u32 mipmaps);

internal void G_TextureDestroy(G_ResourceKey texture);

internal G_Texture *G_TextureFromKey(G_ResourceKey key);


/* ==================================================
   VIEWS
   ================================================== */

internal G_ResourceKey G_TextureViewFetch(const G_TextureViewCreateInfo *create_info);
internal G_ResourceKey G_TextureViewAuto(G_ResourceKey texture);

internal G_TextureView *G_TextureViewFromKey(G_ResourceKey key);

internal u32 G_TextureViewBindless(G_ResourceKey key);


/* ==================================================
   BUFFERS
   ================================================== */

internal G_ResourceKey G_BufferAlloc(const G_BufferAllocInfo *alloc_info);
internal G_ResourceKey G_StageAlloc(u64 size);

internal void G_BufferDestroy(G_ResourceKey buffer);

internal G_Buffer *G_BufferFromKey(G_ResourceKey key);

internal void *G_BufferMap(G_ResourceKey key);
internal u64 G_BufferAddress(G_ResourceKey key);

internal void G_BufferRead(G_ResourceKey key, void *dst, u64 length, u64 offset);
internal void G_BufferWrite(G_ResourceKey key, const void *src, u64 length, u64 offset);

internal u64 G_BufferSize(G_ResourceKey key);


/* ==================================================
   SAMPLERS
   ================================================== */

internal G_ResourceKey G_SamplerCreate(const G_SamplerCreateInfo *create_info);
internal G_ResourceKey G_SamplerCreateF(VkFilter filter);

internal void G_SamplerDestroy(G_ResourceKey key);

internal G_Sampler *G_SamplerFromKey(G_ResourceKey key);

internal u32 G_SamplerBindless(G_ResourceKey key);


/* ==================================================
   SHADERS
   ================================================== */

internal G_ShaderStage G_ShaderStageCreate(Arena *arena, const G_ShaderBytecode *bytecode);

internal G_ResourceKey G_ShaderProgramCreate(u32 stage_count, const G_ShaderBytecode *stages);
internal void G_ShaderProgramDestroy(G_ResourceKey key);

internal G_ShaderProgram *G_ShaderProgramFromKey(G_ResourceKey key);


/* ==================================================
   ACCELERATION STRUCTURES
   ================================================== */

typedef struct G_AllocAccelStructReceipt G_AllocAccelStructReceipt;
struct G_AllocAccelStructReceipt
{
	G_ResourceKey key;
	u64 scratch_size;
};

// Bottom-Level Acceleration Structure
// -- vertex / index data
internal G_AllocAccelStructReceipt G_BLASAlloc(const G_BLASGeometry *geometries, u32 geometry_count);

// Top-Level Acceleration Structure
// -- objects
internal G_AllocAccelStructReceipt G_TLASAlloc(u32 max_instance_count);

internal void G_AccelStructDestroy(G_ResourceKey key);

internal u64 G_AccelStructAddress(G_ResourceKey key);

internal G_AccelStruct *G_AccelStructFromKey(G_ResourceKey key);


/* ==================================================
   IMGUI
   ================================================== */

internal void G_ImGuiNewFrame(void);
internal void G_ImGuiRecord(const G_CmdBuffer *cmd);


#endif // GRAPHICS_DEVICE_H

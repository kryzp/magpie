#ifndef GRAPHICS_DEVICE_H
#define GRAPHICS_DEVICE_H


/* ==================================================
   MACRO BULLSHIT

   Auto-generates a linked list for every
   managed resource in the graphics device.
   ================================================== */

#define G_DEVICE_MANAGED_RESOURCE(mgp_name, resource_name)			\
	typedef struct G_Device##mgp_name##Node G_Device##mgp_name##Node; \
	struct G_Device##mgp_name##Node									\
	{																	\
		G_Device##mgp_name##Node *next;								\
		G_##mgp_name##Key key;										\
		resource_name resource;											\
	};																	\
	typedef struct G_Device##mgp_name##List G_Device##mgp_name##List; \
	struct G_Device##mgp_name##List									\
	{																	\
		G_Device##mgp_name##Node *first;								\
	};																	\
	static G_##mgp_name##Key G_Device##mgp_name##ListPush    (G_Device##mgp_name##List *list, Arena *arena, const resource_name *resource, G_##mgp_name##Key key); \
	static G_##mgp_name##Key G_Device##mgp_name##ListPushAuto(G_Device##mgp_name##List *list, Arena *arena, const resource_name *resource); \
	static resource_name *G_Device##mgp_name##ListGet(const G_Device##mgp_name##List *list, G_##mgp_name##Key key);

#include "graphics_device_managed_resources.inc"

#undef G_DEVICE_MANAGED_RESOURCE


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
	G_TextureKey texture;
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
   DESTRUCTION
   ================================================== */

typedef struct G_DestroyedImage G_DestroyedImage;
struct G_DestroyedImage
{
	G_DestroyedImage *next;
	VkImage image;
	VmaAllocation allocation;
};

typedef struct G_DestroyedBuffer G_DestroyedBuffer;
struct G_DestroyedBuffer
{
	G_DestroyedBuffer *next;
	VkBuffer buffer;
	VmaAllocation allocation;
};

typedef struct G_DestroyedSampler G_DestroyedSampler;
struct G_DestroyedSampler
{
	G_DestroyedSampler *next;
	VkSampler sampler;
	u32 bindless;
};


/* ==================================================
   DEVICE
   ================================================== */

typedef struct G_DevicePerFrameData G_DevicePerFrameData;
struct G_DevicePerFrameData
{
	Arena arena;
	
	G_TimelinePoint completion_point;

	// Non-Timeline Semaphores.
	VkSemaphore image_available_semaphore; // Wait until OS gives us an image.
	VkSemaphore render_finished_semaphore; // Signaled when the OS lets us present.

	G_CmdPool command_pool;

	G_DestroyedImage    *destroyed_image_head;
	G_DestroyedBuffer   *destroyed_buffer_head;
	G_DestroyedSampler  *destroyed_sampler_head;
};

typedef struct G_Device G_Device;
struct G_Device
{
	Arena *permanent_arena;

	LOG_Channel log_channel;
	
	LOG_Channel log_channel_general;
	LOG_Channel log_channel_validation;
	LOG_Channel log_channel_performance;
	
	G_Context context;

	u32 current_frame_index;
	G_DevicePerFrameData per_frame_data[G_FRAMES_IN_FLIGHT];

	G_DevicePipelineLayoutList layouts;
	G_DevicePipelineList pipelines;
	G_DeviceTextureList textures;
	G_DeviceTextureViewList views;
	G_DeviceBufferList buffers;
	G_DeviceSamplerList samplers;
	G_DeviceShaderList shaders;
	G_DeviceAccelStructList accel_structures;
	
	G_Semaphore graphics_semaphore;

	G_Bindless bindless;
	
	VkDescriptorPool imgui_pool;
};


/* ==================================================
   INTERNALS
   ================================================== */

static VkSurfaceFormatKHR G_DeviceChooseSwapchainSurfaceFormat(LOG_Channel channel,
															   u32 available_surface_format_count,
															   const VkSurfaceFormatKHR *available_surface_formats);

static VkPresentModeKHR G_DeviceChooseSwapchainPresentMode(u32 available_present_mode_count,
														   const VkPresentModeKHR *available_present_modes,
														   b32 enable_vsync);

static VkExtent2D G_DeviceChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR *capabilities);
static u32 G_DeviceClampMipmapCount(u32 mipmaps, u32 w, u32 h, u32 d);


/* ==================================================
   CORE DEVICE
   ================================================== */

static void G_DeviceInitAndSelect(G_Device *device, Arena *arena, LOG_Channel log_channel);
static void G_DeviceDestroy(void);

static void G_DeviceSelectContext(G_Device *device);
static G_Device *G_DeviceGetSelected(void);

static VkFormat G_DeviceDepthFormat(void);

static void G_DeviceFlushFrameData(G_DevicePerFrameData *frame_data);

static G_CmdBuffer G_DeviceBeginFrame(G_Swapchain *swapchain);
static void G_DeviceEndFrame(const G_Swapchain *swapchain, const G_CmdBuffer *cmd);

static G_TimelinePoint G_DeviceSubmit(const G_CmdBuffer *cmd);

static G_TimelinePoint G_DeviceSubmitEx(const G_CmdBuffer *cmd,
										u32 wait_count, const VkSemaphoreSubmitInfo *waits,
										u32 signal_count, const VkSemaphoreSubmitInfo *signals);

static G_CmdBuffer G_DeviceSubmitImBegin(void);
static void  G_DeviceSubmitImEnd(const G_CmdBuffer *cmd);

static void G_DeviceHotLoad(void);
static void G_DeviceHotUnload(void);

static void G_DeviceCreateSyncResources(void);
static void G_DeviceDestroySyncResources(void);

static void G_DeviceCreateBindless(void);
static void G_DeviceApplyBindlessUpdates(void);
static void G_DeviceDestroyBindless(void);

static void G_DeviceCreateImGui(void);
static void G_DeviceDestroyImGui(void);


/* ==================================================
   QUERY
   ================================================== */

static void G_DeviceQueryPoolDestroy(VkQueryPool pool);


/* ==================================================
   SYNCHRONISATION
   ================================================== */

static void G_DeviceWaitIdle(void);

static void G_DeviceWaitForFence(VkFence fence);
static void G_DeviceResetFence(VkFence fence);
static void G_DeviceDestroyFence(VkFence fence);

static G_Semaphore G_DeviceSemaphoreCreate(u64 value);
static void G_DeviceSemaphoreDestroy(const G_Semaphore *semaphore);
static u64 G_DeviceSemaphoreGPUCounterValue(const G_Semaphore *semaphore);

static void G_DeviceWaitUntil(G_TimelinePoint point);


/* ==================================================
   SWAPCHAIN
   ================================================== */

static G_Swapchain G_DeviceSwapchainCreate(void);
static void G_DeviceSwapchainDestroy(const G_Swapchain *swapchain);


/* ==================================================
   COMMAND POOL
   ================================================== */

static G_CmdPool G_DeviceCmdPoolCreate(u32 family_index);
static void G_DeviceCmdPoolDestroy(const G_CmdPool *pool);

static G_CmdBuffer G_DeviceCmdPoolAcquire(G_CmdPool *pool);
static void G_DeviceCmdPoolRelease(G_CmdPool *pool, const G_CmdBuffer *cmd, u64 fence_value);
static void G_DeviceCmdPoolPurge(G_CmdPool *pool, u64 fence_value);


/* ==================================================
   PIPELINES
   ================================================== */

static G_PipelineLayoutKey G_DevicePipelineLayoutFetch(G_ShaderKey program);
static VkPipelineLayout G_DevicePipelineLayoutFromKey(G_PipelineLayoutKey key);


typedef struct G_PipelineSt G_PipelineSt;
struct G_PipelineSt
{
	G_PipelineKey pipeline;
	G_PipelineLayoutKey layout;
	VkPipelineBindPoint bind_point;
};

static G_PipelineSt G_DeviceFetchGraphicsPipeline(const G_GraphicsPipelineDef *def);
static G_PipelineSt G_DeviceFetchComputePipeline(const G_ComputePipelineDef *def);

static VkPipeline G_DevicePipelineFromKey(G_PipelineKey key);


/* ==================================================
   TEXTURES
   ================================================== */

static G_TextureKey G_DeviceTextureAlloc(const G_TextureAllocInfo *alloc_info);
static G_TextureKey G_DeviceTextureAlloc2D(u32 width, u32 height, VkFormat format, u32 mipmaps);
static G_TextureKey G_DeviceTextureAlloc2DRW(u32 width, u32 height, VkFormat format, u32 mipmaps);
static G_TextureKey G_DeviceTextureAllocDepth2D(u32 width, u32 height, u32 mipmaps);
static G_TextureKey G_DeviceTextureAllocDepth2DRW(u32 width, u32 height, u32 mipmaps);
static G_TextureKey G_DeviceTextureAllocCubemap(u32 resolution, VkFormat format, u32 mipmaps);
static G_TextureKey G_DeviceTextureAllocCubemapDepth(u32 resolution, u32 mipmaps);

static void G_DeviceTextureDestroy(G_TextureKey texture);

static G_Texture *G_DeviceTextureFromKey(G_TextureKey key);


/* ==================================================
   VIEWS
   ================================================== */

static G_TextureViewKey G_DeviceTextureViewFetch(const G_TextureViewCreateInfo *create_info);
static G_TextureViewKey G_DeviceTextureViewAuto(G_TextureKey texture);

static G_TextureView *G_DeviceTextureViewFromKey(G_TextureViewKey key);

static u32 G_DeviceTextureViewBindless(G_TextureViewKey key);


/* ==================================================
   BUFFERS
   ================================================== */

static G_BufferKey G_DeviceBufferAlloc(const G_BufferAllocInfo *alloc_info);
static G_BufferKey G_DeviceStageAlloc(u64 size);

static void G_DeviceBufferDestroy(G_BufferKey buffer);

static G_Buffer *G_DeviceBufferFromKey(G_BufferKey key);

static void *G_DeviceBufferMap(G_BufferKey key);
static u64 G_DeviceBufferAddress(G_BufferKey key);

static void G_DeviceBufferRead(G_BufferKey key, void *dst, u64 length, u64 offset);
static void G_DeviceBufferWrite(G_BufferKey key, const void *src, u64 length, u64 offset);

static u64 G_DeviceBufferSize(G_BufferKey key);


/* ==================================================
   SAMPLERS
   ================================================== */

static G_SamplerKey G_DeviceSamplerCreate(const G_SamplerCreateInfo *create_info);
static G_SamplerKey G_DeviceSamplerCreateF(VkFilter filter);

static void G_DeviceSamplerDestroy(G_SamplerKey sampler);

static G_Sampler *G_DeviceSamplerFromKey(G_SamplerKey key);

static u32 G_DeviceSamplerBindless(G_SamplerKey key);


/* ==================================================
   SHADERS
   ================================================== */

static G_ShaderStage G_DeviceShaderStageCreate(Arena *arena, const G_ShaderBytecode *bytecode);

static G_ShaderKey G_DeviceShaderProgramCreate(u32 stage_count, const G_ShaderBytecode *stages);
static void G_DeviceShaderProgramDestroy(G_ShaderKey program);

static G_ShaderProgram *G_DeviceShaderProgramFromKey(G_ShaderKey key);


/* ==================================================
   ACCELERATION STRUCTURES
   ================================================== */

typedef struct G_DeviceAllocAccelStructReceipt G_DeviceAllocAccelStructReceipt;
struct G_DeviceAllocAccelStructReceipt
{
	G_AccelStructKey key;
	u64 scratch_size;
};

// Bottom-Level Acceleration Structure
// -- vertex / index data
static G_DeviceAllocAccelStructReceipt G_DeviceBLASAlloc(const G_BLASGeometry *geometries, u32 geometry_count);

// Top-Level Acceleration Structure
// -- objects
static G_DeviceAllocAccelStructReceipt G_DeviceTLASAlloc(u32 max_instance_count);

static void G_DeviceAccelStructDestroy(G_AccelStructKey key);
static u64 G_DeviceAccelStructAddress(G_AccelStructKey key);
static G_AccelStruct *G_DeviceAccelStructFromKey(G_AccelStructKey key);


/* ==================================================
   IMGUI
   ================================================== */

static void G_DeviceImGuiNewFrame(void);
static void G_DeviceImGuiRecord(const G_CmdBuffer *cmd);


#endif // GRAPHICS_DEVICE_H

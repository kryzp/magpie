#ifndef GRAPHICS_DEVICE_H
#define GRAPHICS_DEVICE_H


/* ==================================================
   MACRO BULLSHIT

   Auto-generates a linked list for every
   managed resource in the graphics device.
   ================================================== */

#define GFX_DEVICE_MANAGED_RESOURCE(mgp_name, resource_name)			\
	typedef struct GFX_Device##mgp_name##Node GFX_Device##mgp_name##Node; \
	struct GFX_Device##mgp_name##Node									\
	{																	\
		GFX_Device##mgp_name##Node *next;								\
		GFX_##mgp_name##Key key;										\
		resource_name resource;											\
	};																	\
	typedef struct GFX_Device##mgp_name##List GFX_Device##mgp_name##List; \
	struct GFX_Device##mgp_name##List									\
	{																	\
		GFX_Device##mgp_name##Node *first;								\
	};																	\
	internal GFX_##mgp_name##Key GFX_Device##mgp_name##ListPush(GFX_Device##mgp_name##List *list, Arena *arena, const resource_name *resource); \
	internal resource_name *GFX_Device##mgp_name##ListGet(const GFX_Device##mgp_name##List *list, GFX_##mgp_name##Key key);

#include "graphics_device_managed_resources.inc"

#undef GFX_DEVICE_MANAGED_RESOURCE


/* ==================================================
   ALLOC / CREATION PARAMETERS
   ================================================== */

typedef u32 GFX_TextureAllocFlags;
enum
{
	GFX_TextureAllocFlag_None      = 0,
	GFX_TextureAllocFlag_Transient = 1 << 0,
	GFX_TextureAllocFlag_Storage   = 1 << 1,
	GFX_TextureAllocFlag_Cubemap   = 1 << 2
};

typedef struct GFX_TextureAllocInfo GFX_TextureAllocInfo;
struct GFX_TextureAllocInfo
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

	GFX_TextureAllocFlags flags;
};

typedef struct GFX_TextureViewCreateInfo GFX_TextureViewCreateInfo;
struct GFX_TextureViewCreateInfo
{	
	GFX_TextureKey texture;
	VkImageViewType type;
	GFX_SubresourceRange range;
};

typedef struct GFX_BufferAllocInfo GFX_BufferAllocInfo;
struct GFX_BufferAllocInfo
{
	VkBufferUsageFlags2 usage;
	VmaAllocationCreateFlags flags;
	u64 size;
};

typedef struct GFX_SamplerCreateInfo GFX_SamplerCreateInfo;
struct GFX_SamplerCreateInfo
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

typedef struct GFX_DestroyedImage GFX_DestroyedImage;
struct GFX_DestroyedImage
{
	GFX_DestroyedImage *next;
	VkImage image;
	VmaAllocation allocation;
};

typedef struct GFX_DestroyedView GFX_DestroyedView;
struct GFX_DestroyedView
{
	GFX_DestroyedView *next;
	VkImageView view;
	GFX_BindlessHandle bindless;
};

typedef struct GFX_DestroyedBuffer GFX_DestroyedBuffer;
struct GFX_DestroyedBuffer
{
	GFX_DestroyedBuffer *next;
	VkBuffer buffer;
	VmaAllocation allocation;
};

typedef struct GFX_DestroyedSampler GFX_DestroyedSampler;
struct GFX_DestroyedSampler
{
	GFX_DestroyedSampler *next;
	VkSampler sampler;
	GFX_BindlessHandle bindless;
};


/* ==================================================
   DEVICE
   ================================================== */

typedef struct GFX_DevicePerFrameData GFX_DevicePerFrameData;
struct GFX_DevicePerFrameData
{
	GFX_TimelinePoint completion_point;

	// Non-Timeline Semaphores.
	VkSemaphore image_available_semaphore; // Wait until OS gives us an image.
	VkSemaphore render_finished_semaphore; // Signaled when the OS lets us present.

	GFX_CmdPool command_pool;

	GFX_DestroyedImage    *destroyed_image_head;
	GFX_DestroyedView     *destroyed_view_head;
	GFX_DestroyedBuffer   *destroyed_buffer_head;
	GFX_DestroyedSampler  *destroyed_sampler_head;
};

typedef struct GFX_Device GFX_Device;
struct GFX_Device
{
	Arena *permanent_arena;
	Arena *frame_arena;
	
	GFX_Context context;

	u32 current_frame_index;
	GFX_DevicePerFrameData per_frame_data[GFX_FRAMES_IN_FLIGHT];

	GFX_DevicePipelineLayoutList  layouts;
	GFX_DevicePipelineList        pipelines;
	GFX_DeviceTextureList         textures;
	GFX_DeviceTextureViewList     views;
	GFX_DeviceBufferList          buffers;
	GFX_DeviceSamplerList         samplers;
	GFX_DeviceShaderList          shaders;
	
	GFX_Semaphore graphics_semaphore;

	GFX_Bindless bindless;
	
	VkDescriptorPool imgui_pool;
};


/* ==================================================
   INTERNALS
   ================================================== */

internal VkSurfaceFormatKHR GFX_DeviceChooseSwapchainSurfaceFormat(u32 available_surface_format_count,
																   const VkSurfaceFormatKHR *available_surface_formats);

internal VkPresentModeKHR   GFX_DeviceChooseSwapchainPresentMode(u32 available_present_mode_count,
																 const VkPresentModeKHR *available_present_modes,
																 b32 enable_vsync);

internal VkExtent2D         GFX_DeviceChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR *capabilities);
internal u32                GFX_DeviceClampMipmapCount(u32 mipmaps, u32 w, u32 h, u32 d);


/* ==================================================
   CORE DEVICE
   ================================================== */

internal void GFX_DeviceInit    (GFX_Device *device, Arena *permanent_arena, Arena *frame_arena);
internal void GFX_DeviceDestroy (GFX_Device *device);

internal GFX_CmdBuffer GFX_DeviceBeginFrame (GFX_Device *device, GFX_Swapchain *swapchain);
internal void          GFX_DeviceEndFrame   (GFX_Device *device, const GFX_Swapchain *swapchain, GFX_CmdBuffer *cmd);

internal GFX_TimelinePoint GFX_DeviceSubmit        (GFX_Device *device, GFX_CmdBuffer *cmd);
internal GFX_TimelinePoint GFX_DeviceSubmitEx      (GFX_Device *device, GFX_CmdBuffer *cmd, u32 wait_count, const VkSemaphoreSubmitInfo *waits, u32 signal_count, const VkSemaphoreSubmitInfo *signals);

internal GFX_CmdBuffer     GFX_DeviceSubmitImBegin (GFX_Device *device);
internal void              GFX_DeviceSubmitImEnd   (GFX_Device *device, GFX_CmdBuffer *cmd);

internal void GFX_DeviceHotLoad(GFX_Device *device);
internal void GFX_DeviceHotUnload(GFX_Device *device);

internal void GFX_DeviceCreateSyncResources(GFX_Device *device);
internal void GFX_DeviceDestroySyncResources(GFX_Device *device);

internal void GFX_DeviceCreateBindless(GFX_Device *device);
internal void GFX_DeviceApplyBindlessUpdates(GFX_Device *device);
internal void GFX_DeviceDestroyBindless(GFX_Device *device);

internal void GFX_DeviceDestroyImGui(GFX_Device *device);
internal void GFX_DeviceCreateImGui(GFX_Device *device);


/* ==================================================
   QUERY
   ================================================== */

internal void GFX_DeviceQueryPoolDestroy(const GFX_Device *device, VkQueryPool pool);


/* ==================================================
   SYNCHRONISATION
   ================================================== */

internal void GFX_DeviceWaitIdle(const GFX_Device *device);

internal void GFX_DeviceWaitForFence (const GFX_Device *device, VkFence fence);
internal void GFX_DeviceResetFence   (const GFX_Device *device, VkFence fence);
internal void GFX_DeviceDestroyFence (const GFX_Device *device, VkFence fence);

internal GFX_Semaphore GFX_DeviceSemaphoreCreate  (const GFX_Device *device, u64 value);
internal void          GFX_DeviceSemaphoreDestroy (const GFX_Device *device, const GFX_Semaphore *semaphore);
internal u64           GFX_DeviceSemaphoreValue   (const GFX_Device *device, const GFX_Semaphore *semaphore);

internal void GFX_DeviceWaitUntil(const GFX_Device *device, GFX_TimelinePoint point);


/* ==================================================
   SWAPCHAIN
   ================================================== */

internal GFX_Swapchain GFX_DeviceSwapchainCreate  (const GFX_Device *device);
internal void          GFX_DeviceSwapchainDestroy (const GFX_Device *device, const GFX_Swapchain *swapchain);


/* ==================================================
   COMMAND POOL
   ================================================== */

internal GFX_CmdPool   GFX_DeviceCmdPoolCreate   (const GFX_Device *device, u32 family_index);
internal void          GFX_DeviceCmdPoolDestroy  (const GFX_Device *device, const GFX_CmdPool *pool);
internal void          GFX_DeviceCmdPoolReset    (const GFX_Device *device, GFX_CmdPool *pool);
internal GFX_CmdBuffer GFX_DeviceFetchFreeBuffer (const GFX_Device *device, GFX_CmdPool *pool);


/* ==================================================
   PIPELINES
   ================================================== */

internal GFX_PipelineLayoutKey GFX_DevicePipelineLayoutCreate  (GFX_Device *device, GFX_ShaderKey program);
internal void                  GFX_DevicePipelineLayoutDestroy (GFX_Device *device, GFX_PipelineLayoutKey layout);

internal GFX_PipelineKey GFX_DeviceCreateGraphicsPipeline (GFX_Device *device, const GFX_GraphicsPipelineDef *def, GFX_PipelineLayoutKey layout);
internal GFX_PipelineKey GFX_DeviceCreateComputePipeline  (GFX_Device *device, const GFX_ComputePipelineDef *def, GFX_PipelineLayoutKey layout);

internal void GFX_DeviceDestroyPipeline(GFX_Device *device, GFX_PipelineKey pipeline);

internal VkPipelineLayout GFX_DevicePipelineLayoutFromKey (const GFX_Device *device, GFX_PipelineLayoutKey key);
internal VkPipeline       GFX_DevicePipelineFromKey       (const GFX_Device *device, GFX_PipelineKey key);


/* ==================================================
   TEXTURES
   ================================================== */

internal GFX_TextureKey GFX_DeviceTextureAlloc             (GFX_Device *device, const GFX_TextureAllocInfo *alloc_info);
internal GFX_TextureKey GFX_DeviceTextureAlloc2D           (GFX_Device *device, u32 width, u32 height, VkFormat format, u32 mipmaps);
internal GFX_TextureKey GFX_DeviceTextureAlloc2DRW         (GFX_Device *device, u32 width, u32 height, VkFormat format, u32 mipmaps);
internal GFX_TextureKey GFX_DeviceTextureAllocDepth2D      (GFX_Device *device, u32 width, u32 height, u32 mipmaps);
internal GFX_TextureKey GFX_DeviceTextureAllocDepth2DRW    (GFX_Device *device, u32 width, u32 height, u32 mipmaps);
internal GFX_TextureKey GFX_DeviceTextureAllocCubemap      (GFX_Device *device, u32 resolution, VkFormat format, u32 mipmaps);
internal GFX_TextureKey GFX_DeviceTextureAllocCubemapDepth (GFX_Device *device, u32 resolution, u32 mipmaps);

internal void GFX_DeviceTextureDestroy(GFX_Device *device, GFX_TextureKey texture);

internal GFX_Texture *GFX_DeviceTextureFromKey(const GFX_Device *device, GFX_TextureKey key);


/* ==================================================
   VIEWS
   ================================================== */

internal GFX_TextureViewKey GFX_DeviceTextureViewCreate(GFX_Device *device, const GFX_TextureViewCreateInfo *create_info);
internal GFX_TextureViewKey GFX_DeviceTextureViewAuto(GFX_Device *device, GFX_TextureKey texture);

internal void GFX_DeviceTextureViewDestroy(GFX_Device *device, GFX_TextureViewKey view);

internal GFX_TextureView *GFX_DeviceTextureViewFromKey(const GFX_Device *device, GFX_TextureViewKey key);


/* ==================================================
   BUFFERS
   ================================================== */

internal GFX_BufferKey GFX_DeviceBufferAlloc (GFX_Device *device, const GFX_BufferAllocInfo *alloc_info);
internal GFX_BufferKey GFX_DeviceStageAlloc  (GFX_Device *device, u64 size);

internal void GFX_DeviceBufferDestroy(GFX_Device *device, GFX_BufferKey buffer);

internal GFX_Buffer *GFX_DeviceBufferFromKey(const GFX_Device *device, GFX_BufferKey key);


/* ==================================================
   SAMPLERS
   ================================================== */

internal GFX_SamplerKey GFX_DeviceSamplerCreate  (GFX_Device *device, const GFX_SamplerCreateInfo *create_info);
internal GFX_SamplerKey GFX_DeviceSamplerCreateF (GFX_Device *device, VkFilter filter);

internal void GFX_DeviceSamplerDestroy(GFX_Device *device, GFX_SamplerKey sampler);

internal GFX_Sampler *GFX_DeviceSamplerFromKey(const GFX_Device *device, GFX_SamplerKey key);


/* ==================================================
   SHADERS
   ================================================== */

internal GFX_ShaderStage GFX_DeviceShaderStageCreate(Arena *arena, const GFX_ShaderBytecode *bytecode);

internal GFX_ShaderKey GFX_DeviceShaderProgramCreate(GFX_Device *device, u32 stage_count, const GFX_ShaderBytecode *stages);
internal void          GFX_DeviceShaderProgramDestroy(GFX_Device *device, GFX_ShaderKey program);

internal GFX_ShaderProgram *GFX_DeviceShaderProgramFromKey(const GFX_Device *device, GFX_ShaderKey key);


/* ==================================================
   IMGUI
   ================================================== */

internal void GFX_DeviceImGuiNewFrame (const GFX_Device *device);
internal void GFX_DeviceImGuiRecord   (const GFX_Device *device, const GFX_CmdBuffer *cmd);


#endif // GRAPHICS_DEVICE_H

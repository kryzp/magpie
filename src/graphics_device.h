
#define FRAMES_IN_FLIGHT 3

typedef struct Sampler
{
	VkSampler handle;
	
	VkFilter filter;
	
	VkSamplerAddressMode wrap_x;
	VkSamplerAddressMode wrap_y;
	VkSamplerAddressMode wrap_z;
	
	VkBorderColor border_colour;
}
Sampler;

typedef struct Image
{
	VkImage image;
	VkImageLayout layout;
	
	u32 width;
	u32 height;
	u32 depth;
	
	VkFormat format;
	VkImageViewType type;
	VkImageTiling tiling;
	
	VkImageUsageFlags usage;
	
	u32 mipmap_count;
	VkSampleCountFlagBits samples;
	
	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
}
Image;

typedef struct ImageView
{
	Image *image;
	VkImageView view;
	
	u32 layer_count;
	u32 layer;
	u32 base_mip_level;
}
ImageView;

typedef struct GPUBuffer
{
	VkBuffer handle;
	VkBufferUsageFlags usage;
	
	u64 size;
	
	VkDeviceAddress device_address;
	
	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
	VmaAllocationCreateFlagBits allocation_flags;
}
GPUBuffer;

typedef struct ShaderStage
{
	VkShaderStageFlagBits stage;
	VkShaderModule module;
}
ShaderStage;

typedef struct ShaderProgram
{
	u64 push_constant_size;
	
	u32 layout_count;
	VkDescriptorSetLayout layouts[8];
	
	u32 stage_count;
	ShaderStage stages[2];
}
ShaderProgram;

#define BINDLESS_COMBINED_IMAGE_BINDING 0
#define BINDLESS_MAX_WRITES_PER_FRAME 16

typedef struct BindlessCombinedImageUpdate
{
	u32 slot;
	ImageView *view;
	Sampler *sampler;
}
BindlessCombinedImageUpdate;

typedef struct BindlessResources
{
	VkDescriptorSet bindless_set;
	VkDescriptorSetLayout bindless_layout;
	VkDescriptorPool bindless_pool;
	
	u32 n_combined_image_updates;
	BindlessCombinedImageUpdate combined_image_updates[BINDLESS_MAX_WRITES_PER_FRAME];
}
BindlessResources;

#define MAX_VERTEX_ATTRIBUTES 32

typedef struct VertexFormat
{
	u32 attribute_count;
	VkVertexInputAttributeDescription attributes[MAX_VERTEX_ATTRIBUTES];
	
	u32 binding_count;
	VkVertexInputBindingDescription bindings[2];
	
	u64 vertex_size;
	u64 instance_size;
}
VertexFormat;

typedef struct Blend
{
	VkBlendOp op;
	VkBlendFactor src;
	VkBlendFactor dst;
}
Blend;

typedef struct BlendState
{
	b32 enabled;
	
	// [r, g, b, a]
	f32 constants[4];
	b32 write_mask[4];
	
	Blend colour;
	Blend alpha;
	
	b32 logic_op_enabled;
	VkLogicOp logic_op;
}
BlendState;

typedef struct StencilState
{
	VkStencilOp fail_op;
	VkStencilOp pass_op;
	VkStencilOp depth_fail_op;
	
	VkCompareOp compare_op;
	
	u32 write_mask;
	u32 reference;
}
StencilState;

typedef struct DepthStencilState
{
	b32 depth_test_enabled;
	b32 depth_write_enabled;
	VkCompareOp depth_compare_op;
	b32 depth_bounds_test_enabled;
	f32 depth_bounds_min;
	f32 depth_bounds_max;
	
	b32 stencil_test_enabled;
	StencilState stencil_front;
	StencilState stencil_back;
}
DepthStencilState;

#define MAX_COLOUR_ATTACHMENTS 32

typedef struct RenderInfo
{
	u32 width;
	u32 height;
	
	VkSampleCountFlagBits samples;
	
	u32 colour_attachment_count;
	VkRenderingAttachmentInfo colour_attachments[MAX_COLOUR_ATTACHMENTS];
	
	VkRenderingAttachmentInfo depth_attachment;
}
RenderInfo;

typedef struct GraphicsPipelineDef
{
	ShaderProgram *program;
	VertexFormat *vertex_format;
	
	VkCullModeFlags cull_mode;
	VkFrontFace front_face;
	
	BlendState blend_state;
	DepthStencilState depth_stencil_state;
	
	u32 colour_attachment_count;
	VkFormat colour_attachment_formats[MAX_COLOUR_ATTACHMENTS];
	
	b32 has_depth_attachment;
	
	VkSampleCountFlagBits samples;
	
	b32 min_sample_shading_enabled;
	f32 min_sample_shading;
}
GraphicsPipelineDef;

typedef struct ComputePipelineDef
{
	ShaderProgram *program;
}
ComputePipelineDef;

typedef struct PipelineState
{
	VkPipeline pipeline;
	VkPipelineLayout layout;
}
PipelineState;

typedef struct CommandBuffer
{
	VkCommandBuffer handle;
}
CommandBuffer;

#define COMMAND_POOL_FREE_BUFFER_COUNT 16

typedef struct CommandPool
{
	VkCommandPool handle;
	
	u32 free_index;
	VkCommandBuffer free_buffers[COMMAND_POOL_FREE_BUFFER_COUNT];
}
CommandPool;

typedef struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	
	u32 surface_format_count;
	VkSurfaceFormatKHR *surface_formats;
	
	u32 present_mode_count;
	VkPresentModeKHR *present_modes;
}
SwapchainSupportDetails;

typedef struct Swapchain
{
	VkSwapchainKHR handle;
	
	// NOTE(kp): This is *DIFFERENT* from GraphicsDevice::current_frame_index!
	u32 current_image_index;
	
	u32 width;
	u32 height;
	
	VkFormat format;
	
	u32 swapchain_image_count;
	Image *swapchain_images;
	ImageView *swapchain_image_views;
}
Swapchain;

typedef struct GraphicsFrameData
{
	CommandPool command_pool;
	
	VkFence in_flight_fence;
	VkFence instant_submit_fence;
	
	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
}
GraphicsFrameData;

typedef struct GraphicsDevice
{
	VkInstance instance;
	VkDevice device;
	
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties2 physical_device_properties;
	VkPhysicalDeviceFeatures2 physical_device_features;
	
	u32 current_frame_index;
	
	Swapchain swapchain;
	BindlessResources bindless;
	
	VkQueue graphics_queue;
	u32 graphics_queue_family_index;
	
	GraphicsFrameData frames[FRAMES_IN_FLIGHT];
	
	VkSurfaceKHR surface;
	VkPipelineCache pipeline_process_cache;
	
	VkFormat depth_format;
	VkSampleCountFlagBits max_msaa_samples;
	
	VmaAllocator vma_allocator;
	
	VkDebugUtilsMessengerEXT debug_messenger;
	b32 has_validation_layers;
}
GraphicsDevice;

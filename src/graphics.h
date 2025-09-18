
#define FRAMES_IN_FLIGHT 3

#define VK_CHECK(_func_call, _error_msg)                \
	do {                                            \
		VkResult _vk_check_result = _func_call; \
		if (_vk_check_result != VK_SUCCESS) {   \
			DebugLogCrash(_error_msg);      \
		}                                       \
	} while (0)

#define BINDLESS_MAX_RESOURCES 256
#define BINDLESS_MAX_WRITES_PER_FRAME 64

typedef enum BindlessSetBinding {
	BindlessSetBinding_Sampler,
	BindlessSetBinding_Sampled,
	BindlessSetBinding_Storage,
	BindlessSetBinding_MaxEnum
} BindlessSetBinding;

typedef u32 bindless_handle;

typedef struct BindlessSamplerHandle {
	bindless_handle id;
} BindlessSamplerHandle;

typedef struct BindlessImageHandle {
	bindless_handle sampled;
	bindless_handle storage;
} BindlessImageHandle;

typedef struct BindlessUpdate {
	BindlessSetBinding type;
	bindless_handle slot;
	union {
		VkSampler sampler;
		VkImageView view;
	};
} BindlessUpdate;

typedef struct BindlessResources {
	VkDescriptorSet sets[BindlessSetBinding_MaxEnum];
	VkDescriptorSetLayout layouts[BindlessSetBinding_MaxEnum];
	VkDescriptorPool pool;

	u32 update_count;
	BindlessUpdate updates[BINDLESS_MAX_WRITES_PER_FRAME];

	u32 resource_counts[BindlessSetBinding_MaxEnum];
} BindlessResources;

typedef struct Sampler {
	VkSampler handle;

	VkFilter filter;

	VkSamplerAddressMode wrap_x;
	VkSamplerAddressMode wrap_y;
	VkSamplerAddressMode wrap_z;

	VkBorderColor border_colour;

	BindlessSamplerHandle bindless;
} Sampler;

typedef enum ImageAccessType {
	ImageAccessType_Undefined,
	ImageAccessType_General,
	ImageAccessType_GraphicsRead,
	ImageAccessType_GraphicsReadWrite,
	ImageAccessType_ComputeRead,
	ImageAccessType_ComputeReadWrite,
	ImageAccessType_ColourWrite,
	ImageAccessType_DepthWrite,
	ImageAccessType_BlitSrc,
	ImageAccessType_BlitDst,
	ImageAccessType_CopySrc,
	ImageAccessType_CopyDst,
	ImageAccessType_Present,
	ImageAccessType_MaxEnum
} ImageAccessType;

typedef struct ImageAccessInfo {
	VkImageLayout layout;
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
} ImageAccessInfo;

typedef struct Image {
	VkImage handle;
	VkImageUsageFlags usage;

	u32 access_count;

	// Access types are arranged into a 3D matrix: MIPS x LAYERS x ASPECTS.
	// TODO: Use a proper blobbing algorithm for generating pipeline barriers.
	//       Right now I just go through the columns of the matrix.
	//       --> Could be a fun project.
	ImageAccessType *access_types;

	u32 width;
	u32 height;
	u32 depth;

	b32 is_swapchain;

	VkFormat format;
	VkImageViewType type;
	VkImageTiling tiling;

	u32 aspect_count;
	VkImageAspectFlags aspect_flags;
	
	u32 mipmap_count;
	
	VkSampleCountFlagBits samples;

	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
} Image;

typedef struct ImageView {
	VkImageView handle;
	
	Image *image;

	u32 layer_count;
	u32 layer;
	u32 mip_level_count;
	u32 base_mip_level;

	VkImageAspectFlags aspect;

	BindlessImageHandle bindless;
} ImageView;

typedef enum GPUBufferAccessType {
	GPUBufferAccessType_Undefined,
	GPUBufferAccessType_GraphicsReadWrite,
	GPUBufferAccessType_ComputeReadWrite,
	GPUBufferAccessType_CopySrc,
	GPUBufferAccessType_CopyDst,
	GPUBufferAccessType_IndirectDraw,
	GPUBufferAccessType_MaxEnum
} GPUBufferAccessType;

typedef struct GPUBufferAccessInfo {
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
} GPUBufferAccessInfo;

typedef struct GPUBuffer {
	VkBuffer handle;
	VkBufferUsageFlags2 usage;
	
	GPUBufferAccessType access_type;

	u64 size;

	VkDeviceAddress device_address;

	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
	VmaAllocationCreateFlagBits allocation_flags;
} GPUBuffer;

typedef struct ShaderStage {
	VkShaderStageFlagBits stage;
	VkShaderModule module;
} ShaderStage;

// Shaders are fully bindless.
// --> No layouts per shader.
typedef struct ShaderProgram {
	u32 push_constant_size;
	u32 stage_count;
	ShaderStage stages[2];
} ShaderProgram;

typedef struct VertexFormat {
	u32 binding_count;
	VkVertexInputBindingDescription bindings[2];

	u32 attribute_count;
	VkVertexInputAttributeDescription attributes[32];

	u64 vertex_size;
	u64 instance_size;
} VertexFormat;

typedef struct Blend {
	VkBlendOp op;
	VkBlendFactor src;
	VkBlendFactor dst;
} Blend;

typedef struct BlendState {
	b32 enabled;

	// [r, g, b, a]
	f32 constants[4];
	b32 write_mask[4];

	Blend colour;
	Blend alpha;

	b32 logic_op_enabled;
	VkLogicOp logic_op;
} BlendState;

typedef struct StencilState {
	VkStencilOp fail_op;
	VkStencilOp pass_op;
	VkStencilOp depth_fail_op;

	VkCompareOp compare_op;

	u32 write_mask;
	u32 reference;
} StencilState;

typedef struct DepthStencilState {
	b32 depth_test_enabled;
	b32 depth_write_enabled;

	VkCompareOp depth_compare_op;

	b32 depth_bounds_test_enabled;
	f32 depth_bounds_min;
	f32 depth_bounds_max;

	b32 stencil_test_enabled;
	StencilState stencil_front;
	StencilState stencil_back;
} DepthStencilState;

#define MAX_COLOUR_ATTACHMENTS 32

typedef struct RenderInfo {
	u32 width;
	u32 height;

	VkSampleCountFlagBits samples;

	u32 view_mask;

	u32 colour_attachment_count;
	VkRenderingAttachmentInfo colour_attachments[MAX_COLOUR_ATTACHMENTS];

	VkRenderingAttachmentInfo depth_attachment;
} RenderInfo;

typedef struct GraphicsPipelineDef {
	ShaderProgram *program;

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

	u32 view_mask;
} GraphicsPipelineDef;

typedef struct ComputePipelineDef {
	ShaderProgram *program;
} ComputePipelineDef;

typedef struct PipelineState {
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkPipelineBindPoint bind_point;
} PipelineState;

typedef struct CommandBuffer {
	VkCommandBuffer handle;
} CommandBuffer;

internal void CmdPipelineBarrier(CommandBuffer *cmd,
				 VkDependencyFlags dependency_flags,
				 u32 memory_barrier_count,        VkMemoryBarrier2       *memory_barriers,
				 u32 buffer_memory_barrier_count, VkBufferMemoryBarrier2 *buffer_memory_barriers,
				 u32 image_memory_barrier_count,  VkImageMemoryBarrier2  *image_memory_barriers);

typedef struct CommandPool {
	VkCommandPool handle;
	u32 free_index;
	VkCommandBuffer free_buffers[64];
} CommandPool;

typedef struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;

	u32 surface_format_count;
	VkSurfaceFormatKHR *surface_formats;

	u32 present_mode_count;
	VkPresentModeKHR *present_modes;
} SwapchainSupportDetails;

typedef struct Swapchain {
	VkSwapchainKHR handle;

	// This is *DIFFERENT* from GraphicsDevice::current_frame_index!
	// A swapchain might have, e.g: 3 frames while the graphics
	// device only has 2 frames in flight. They are *usually* the same
	// but not always!
	u32 current_image_index;

	u32 width;
	u32 height;

	VkFormat format;

	u32 swapchain_image_count;
	Image *swapchain_images;
	ImageView *swapchain_image_views;
} Swapchain;

typedef struct GraphicsFrameData {
	CommandPool command_pool;

	VkFence in_flight_fence;
	VkFence instant_submit_fence;

	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
} GraphicsFrameData;

typedef struct GraphicsDevice {
	MemoryArena *arena;

	// ---
	
	VkInstance instance;
	VkDevice device;

	// ---

	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties2 physical_device_properties;
	VkPhysicalDeviceFeatures2 physical_device_features;

	// ---

	VmaAllocator vma_allocator;

	// ---

	VkDebugUtilsMessengerEXT debug_messenger;
	b32 has_validation_layers;

	// ---

	u32 current_frame_index;

	// ---

	Swapchain swapchain;
	BindlessResources bindless;

	GraphicsFrameData frames[FRAMES_IN_FLIGHT];

	VkQueue graphics_queue;
	u32 graphics_queue_family_index;

	VkSurfaceKHR surface;
	VkPipelineCache pipeline_process_cache;

	VkFormat depth_format;
	VkSampleCountFlagBits max_msaa_samples;
	
	// ---

	HashTable image_view_cache;
	HashTable pipeline_cache;
	HashTable pipeline_layout_cache;
} GraphicsDevice;

internal CommandBuffer GraphicsBeginInstantSubmit();
internal void GraphicsEndInstantSubmit(CommandBuffer *cmd);
internal void GraphicsWaitIdle();

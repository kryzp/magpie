
#define FRAMES_IN_FLIGHT 3

typedef struct Sampler {
	VkSampler handle;

	VkFilter filter;

	VkSamplerAddressMode wrap_x;
	VkSamplerAddressMode wrap_y;
	VkSamplerAddressMode wrap_z;

	VkBorderColor border_colour;

	u32 resource_id;
} Sampler;

typedef struct Image {
	VkImage image;
	VkImageLayout layout;
	VkImageUsageFlags usage;

	u32 width;
	u32 height;
	u32 depth;

	b32 is_swapchain;

	VkFormat format;
	VkImageViewType type;
	VkImageTiling tiling;

	u32 mipmap_count;
	VkSampleCountFlagBits samples;

	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
} Image;

typedef struct ImageView {
	Image *image;
	VkImageView view;

	u32 layer_count;
	u32 layer;
	u32 base_mip_level;

	u32 resource_id;
} ImageView;

typedef struct GPUBuffer {
	VkBuffer handle;
	VkBufferUsageFlags usage;

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

	u32 view_mask;
} GraphicsPipelineDef;

typedef struct ComputePipelineDef {
	ShaderProgram *program;
} ComputePipelineDef;

typedef struct PipelineState {
	VkPipeline pipeline;
	VkPipelineLayout layout;
} PipelineState;

typedef struct CommandBuffer {
	VkCommandBuffer handle;
} CommandBuffer;

typedef struct CommandPool {
	VkCommandPool handle;

	u32 free_index;
	VkCommandBuffer free_buffers[16];
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

#define BINDLESS_MAX_RESOURCES 256
#define BINDLESS_MAX_WRITES_PER_FRAME 64

#define BINDLESS_SAMPLER_BINDING 0
#define BINDLESS_IMAGE_BINDING 1
#define BINDLESS_CUBEMAP_BINDING 2

typedef enum BindlessUpdateType {
	BindlessUpdateType_Sampler,
	BindlessUpdateType_Image,
	BindlessUpdateType_Cubemap
} BindlessUpdateType;

typedef struct BindlessUpdate {
	BindlessUpdateType type;
	u32 slot;

	union {
		struct {
			VkImageView view;
			b32 is_depth;
		} sampled_image;

		struct {
			VkSampler sampler;
		} sampler;
	};
} BindlessUpdate;

typedef struct GraphicsFrameData {
	CommandPool command_pool;

	VkFence in_flight_fence;
	VkFence instant_submit_fence;

	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
} GraphicsFrameData;

typedef struct GraphicsDevice {
	VkInstance instance;
	VkDevice device;

	// ---

	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties2 physical_device_properties;
	VkPhysicalDeviceFeatures2 physical_device_features;

	// ---

	u32 current_frame_index;

	Swapchain swapchain;

	// ---

	VkDescriptorSet bindless_set;
	VkDescriptorSetLayout bindless_layout;
	VkDescriptorPool bindless_pool;

	u32 n_bindless_updates;
	BindlessUpdate bindless_updates[BINDLESS_MAX_WRITES_PER_FRAME];

	u32 n_bindless_samplers;
	u32 n_bindless_images;
	u32 n_bindless_cubemaps;

	// ---

	HashTable image_view_cache;
	HashTable pipeline_cache;
	HashTable pipeline_layout_cache;

	// ---

	VkQueue graphics_queue;
	u32 graphics_queue_family_index;

	GraphicsFrameData frames[FRAMES_IN_FLIGHT];

	VkSurfaceKHR surface;
	VkPipelineCache pipeline_process_cache;

	// ---

	VkFormat depth_format;
	VkSampleCountFlagBits max_msaa_samples;

	// ---

	VmaAllocator vma_allocator;

	// ---

	VkDebugUtilsMessengerEXT debug_messenger;
	b32 has_validation_layers;
} GraphicsDevice;

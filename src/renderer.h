
typedef enum RenderPassType
{
	RenderPassType_Render,
	RenderPassType_Compute
}
RenderPassType;

typedef struct RenderAttachment
{
	ImageView *view;
	ImageView *resolve;
	
	VkResolveModeFlagBits resolve_mode;
	
	VkAttachmentLoadOp load_op;
	VkAttachmentStoreOp store_op;
	
	v4 clear_colour;
	f32 clear_depth;
	u32 clear_stencil;
}
RenderAttachment;

typedef struct RenderPassDef
{
	void (*Record)(CommandBuffer *cmd, const RenderInfo *info);
	
	u32 attachment_count;
	RenderAttachment attachments[32];
	
	u32 view_count;
	ImageView views[32];
}
RenderPassDef;

typedef struct ComputePassDef
{
	void (*Record)(CommandBuffer *cmd);
	
	u32 storage_view_count;
	ImageView *storage_views;
}
ComputePassDef;

typedef struct RenderPassHandle
{
	RenderPassType type;
	u32 index;
}
RenderPassHandle;

#define MAX_RENDER_PASSES 32
#define MAX_COMPUTE_PASSES 32
#define MAX_PASSES_TOTAL (MAX_RENDER_PASSES + MAX_COMPUTE_PASSES)

typedef struct Renderer
{
	CommandBuffer present_cmd;
	
	u32 render_pass_count;
	RenderPassHandle passes[MAX_PASSES_TOTAL];
	
	RenderPassDef internal_render_passes[MAX_RENDER_PASSES];
	ComputePassDef internal_compute_passes[MAX_COMPUTE_PASSES];
	
	VertexFormat my_vertex_format;
	GPUBuffer my_vertex_buffer;
	GPUBuffer my_index_buffer;
	ShaderProgram my_shader_program;
	VkPipeline my_pipeline;
	VkPipelineLayout my_pipeline_layout;
	Image my_image;
	ImageView my_image_view;
	Sampler my_sampler;
}
Renderer;

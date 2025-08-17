
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

typedef struct Renderer Renderer;

typedef struct RenderPassDef
{
	void (*Record)(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info);
	
	u32 attachment_count;
	RenderAttachment attachments[32];
	
	u32 view_count;
	ImageView views[32];
	
	u32 view_mask;
}
RenderPassDef;

typedef struct ComputePassDef
{
	void (*Record)(Renderer *renderer, CommandBuffer *render_info);
	
	u32 storage_view_count;
	ImageView *storage_views;
}
ComputePassDef;

typedef struct Mesh
{
	VertexFormat *vertex_format;
	
	GPUBuffer vertex_buffer;
	GPUBuffer index_buffer;
	
	u32 vertex_count;
	u32 index_count;
}
Mesh;

typedef struct Renderer
{
	CommandBuffer present_cmd;
	
	u32 pass_count;
	
	struct
	{
		RenderPassType type;
		u32 index;
	}
	passes[32];
	
	u32 internal_render_pass_count;
	RenderPassDef internal_render_passes[32];
	
	u32 internal_compute_pass_count;
	ComputePassDef internal_compute_passes[32];
	
	VertexFormat environment_cube_vertex_format;
	Mesh environment_cube_mesh;
	ShaderProgram environment_map_program;
	Image environment_map_hdr_image;
	ImageView environment_map_hdr_image_view;
	Image environment_map_cubemap;
	ImageView environment_map_cubemap_view;
	Sampler environment_map_sampler;
	VkPipelineLayout environment_map_pipeline_layout;
	VkPipeline environment_map_pipeline;
}
Renderer;

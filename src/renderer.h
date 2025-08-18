
typedef struct Renderer Renderer;

typedef struct RenderingAttachment
{
	VkRenderingAttachmentInfo info;
	Image *image;
	u32 width;
	u32 height;
}
RenderingAttachment;

typedef enum RenderPassType
{
	RenderPassType_Graphics,
	RenderPassType_Compute
}
RenderPassType;

typedef struct RenderPass
{
	RenderPassType type;
	u8 context[128];
	
	union
	{
		struct
		{
			void (*Record)(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context);
			
			u32 view_mask;
			
			u32 attachment_count;
			RenderingAttachment attachments[32];
			
			u32 view_count;
			ImageView *views[32];
		}
		graphics;
		
		struct ComputePassDef
		{
			void (*Record)(Renderer *renderer, CommandBuffer *cmd, void *context);
			
			u32 view_count;
			ImageView *views[32];
		}
		compute;
	};
}
RenderPass;

typedef struct Mesh
{
	VertexFormat *vertex_format;
	
	GPUBuffer vertex_buffer;
	GPUBuffer index_buffer;
	
	u32 vertex_count;
	u32 index_count;
}
Mesh;

typedef struct EnvironmentProbe
{
	Image irradiance, prefilter;
}
EnvironmentProbe;

typedef struct Renderer
{
	CommandBuffer present_cmd;
	
	// ---
	
	Sampler linear_sampler;
	
	// ---
	
	VertexFormat environment_cube_vertex_format;
	Mesh environment_cube_mesh;
	GPUBuffer cubemap_capture_transforms;
	
	// ---
	
	ShaderProgram environment_hdr_to_cubemap_program;
	VkPipelineLayout environment_hdr_to_cubemap_pipeline_layout;
	VkPipeline environment_hdr_to_cubemap_pipeline;
	
	Image environment_hdr_image;
	Image environment_cubemap;
	
	// ---
	
	ShaderProgram irradiance_map_program;
	VkPipelineLayout irradiance_map_pipeline_layout;
	VkPipeline irradiance_map_pipeline;
	
	ShaderProgram prefilter_map_program;
	VkPipelineLayout prefilter_map_pipeline_layout;
	VkPipeline prefilter_map_pipeline;
	
	EnvironmentProbe environment_probe;
	
	// ---
	
	u32 pass_count;
	RenderPass passes[32];
}
Renderer;

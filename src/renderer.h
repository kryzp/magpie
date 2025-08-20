
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
	
	// NOTE(kp): Generic data that can be set depending on whatever is required
	//           by the pass. This is then given to the Record(...) function
	//           as the "context".
	u8 context[128];
	
	union
	{
		struct
		{
			void (*Record)(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context);
			
			u32 view_mask;
			
			u32 attachment_count;
			RenderingAttachment attachments[8];
			
			u32 view_count;
			ImageView *views[16];
		}
		graphics;
		
		struct ComputePassDef
		{
			void (*Record)(Renderer *renderer, CommandBuffer *cmd, void *context);
			
			u32 view_count;
			ImageView *views[16];
		}
		compute;
	};
}
RenderPass;

typedef struct EnvironmentCubeVertex
{
	v3 position;
}
EnvironmentCubeVertex;

typedef struct EnvironmentProbe
{
	Image irradiance, prefilter;
}
EnvironmentProbe;

typedef struct Renderer
{
	CommandBuffer present_cmd;
	
	// ---
	
	u32 pass_count;
	RenderPass passes[32];
	
	// ---
	
	Image depth_buffer;
	
	// ---
	
	Sampler linear_sampler;
	
	// ---
	
	VertexFormat model_vertex_format;
	ShaderProgram model_program;
	Model damaged_helmet_model;
	
	// ---
	
	VertexFormat environment_cube_vertex_format;
	Mesh environment_cube_mesh;
	GPUBuffer cubemap_capture_transforms;
	
	ShaderProgram environment_hdr_to_cubemap_program;
	
	Image environment_hdr_image;
	Image environment_cubemap;
	
	ShaderProgram irradiance_map_program;
	ShaderProgram prefilter_map_program;
	
	EnvironmentProbe environment_probe;
}
Renderer;

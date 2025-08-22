
typedef struct GPU_FrameData
{
	m4 view;
	m4 projection;
	m4 view_projection;
	m4 view_projection_no_translation;
	
	m4 inv_view;
	m4 inv_projection;
	
	v4 camera_position;
	
	v4 window_resolution;
	
	f32 time;
	f32 _padding[3];
}
GPU_FrameData;

typedef struct GPU_TransformData
{
	m4 model_matrix;
	m4 normal_matrix;
}
GPU_TransformData;

typedef struct GPU_Light
{
	v4 position;    // x,y,z: position,    w: n/a
	v4 colour;      // x,y,z: colour,      w: intensity
	v4 attenuation; // x,y,z: attenuation, w: n/a
}
GPU_Light;

// ---

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

typedef struct EnvironmentProbe
{
	Image irradiance, prefilter;
}
EnvironmentProbe;

typedef enum GBufferAttachment
{
	GBufferAttachment_Position,
	GBufferAttachment_Albedo,
	GBufferAttachment_Normal,
	GBufferAttachment_Material,
	GBufferAttachment_Emissive,
	GBufferAttachment_MaxEnum
}
GBufferAttachment;

typedef struct GBuffer
{
	Image attachments[GBufferAttachment_MaxEnum];
	Image depth;
}
GBuffer;

// NOTE(kp): This is how you actually get the
//           renderer to draw something from
//           the outside.
typedef struct RenderCall
{
	Mesh *mesh;
	Material *material;
	m4 transform;
}
RenderCall;

typedef struct Renderer
{
	CommandBuffer present_cmd;
	
	// ---
	
	u32 pass_count;
	RenderPass passes[32];
	
	u32 render_call_count;
	RenderCall render_calls[32];
	
	u32 light_count;
	Light lights[16];
	
	Camera *active_camera;
	
	// ---
	
	Sampler linear_sampler;
	
	GBuffer gbuffer;
	
	GPUBuffer frame_data_buffer;
	GPUBuffer transform_data_buffer;
	GPUBuffer light_buffer;
	
	ShaderProgram ambient_lighting_program;
	ShaderProgram direct_lighting_point_program;
	ShaderProgram model_program;
	ShaderProgram environment_hdr_to_cubemap_program;
	ShaderProgram irradiance_map_program;
	ShaderProgram prefilter_map_program;
	ShaderProgram skybox_program;
	ShaderProgram brdf_lut_program;
	
	Mesh skybox_mesh;
	Mesh light_sphere_mesh;
	
	GPUBuffer cubemap_capture_transforms;
	
	Image brdf_lut_image;
	Image environment_hdr_texture;
	Image environment_cubemap;
	
	EnvironmentProbe environment_probe;
}
Renderer;

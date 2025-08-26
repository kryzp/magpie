
typedef struct RenderMesh
{
	Mesh *original;
	
	b32 is_merged;
	
	u32 first_vertex;
	u32 first_index;
	u32 index_count;
}
RenderMesh;

// NOTE(kp): Per-frame data like shader buffers.
typedef struct RenderContextPerFrameData
{
	GPUBuffer frame_data_buffer;
	GPUBuffer object_buffer;
	GPUBuffer light_buffer;
	GPUBuffer indirect_buffer;
}
RenderContextPerFrameData;

typedef struct RenderContext
{
	CommandBuffer cmd;
	
	u32 mesh_count;
	RenderMesh meshes[128];
	
	u32 material_count;
	Material materials[128];
	
	// TODO(kp): Move image view cache and pipeline cache into here.
	
	MeshPass mesh_pass;
	
	GPUBuffer material_buffer;
	
	RenderContextPerFrameData per_frame_data[FRAMES_IN_FLIGHT];
	
	Sampler linear_sampler;
	
	ShaderProgram ambient_lighting_program;
	ShaderProgram direct_lighting_point_program;
	ShaderProgram model_program;
	ShaderProgram hdr_to_environment_cubemap_program;
	ShaderProgram irradiance_map_program;
	ShaderProgram prefilter_map_program;
	ShaderProgram skybox_program;
	ShaderProgram brdf_lut_program;
	
	Mesh skybox_mesh;
	Mesh light_sphere_mesh;
	
	GPUBuffer cubemap_capture_transforms;
	
	Image brdf_lut_image;
	
	// TODO(kp): Move into scene.
	Image skybox_cubemap;
	EnvironmentProbe environment_probe;
}
RenderContext;

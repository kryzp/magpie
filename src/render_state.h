
typedef struct RenderMesh {
	Mesh *original;

	b32 is_merged;

	u32 first_vertex;
	u32 first_index;
	u32 index_count;
} RenderMesh;

typedef struct RenderStateFrameData {
	GPUBuffer frame_data_buffer;
	GPUBuffer object_buffer;
	GPUBuffer light_buffer;
	GPUBuffer indirect_buffer;
} RenderStateFrameData;

// Maintains rendering state that gets
// used between multiple renderers.
// I.e: Anything needed to render the scene,
//      that varies after init.
// --> Doesn't contain per-renderer information.
// --> Doesn't contain permanent "static" data,
//     like shaders or vertex formats.
typedef struct RenderState {
	CommandBuffer cmd;

	u32 mesh_count;
	RenderMesh meshes[128];

	u32 material_count;
	Material materials[128];

	GPUBuffer material_buffer;

	u32 light_count;
	Light lights[16];

	MeshPass mesh_pass;
	
	RenderStateFrameData per_frame_data[FRAMES_IN_FLIGHT];

	// TODO: Move image view cache into here when adding
	//       automatic mip-mapping into the render graph.
} RenderState;

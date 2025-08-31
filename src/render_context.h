
typedef struct RenderMesh {
	Mesh *original;

	b32 is_merged;

	u32 first_vertex;
	u32 first_index;
	u32 index_count;
} RenderMesh;

// NOTE(kp): Maintains rendering state that gets
//           used between multiple renderers.
//           I.e: Anything needed to render the scene,
//                that varies after init.
//           --> Doesn't contain per-renderer information.
//           --> Doesn't contain permanent "static" data,
//               like shaders or vertex formats.
typedef struct RenderContext {
	CommandBuffer cmd;

	u32 mesh_count;
	RenderMesh meshes[128];

	u32 material_count;
	Material materials[128];

	GPUBuffer material_buffer;

	MeshPass mesh_pass;

	// TODO(kp): Move image view cache into here when adding
	//           automatic mip-mapping into the render graph.
} RenderContext;

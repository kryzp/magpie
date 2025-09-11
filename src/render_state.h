
typedef struct RenderMesh {
	Mesh *original;
	b32 is_merged;
	u32 first_vertex;
	u32 first_index;
	u32 vertex_count;
	u32 index_count;
} RenderMesh;

// TODO: Currently meshes and materials just use an indexed handle,
//       which is all well and good but becomes troublesome when
//       removing things, so fix that :/.

typedef struct RenderState {
	CommandBuffer cmd;

	u32 mesh_count;
	RenderMesh meshes[SCENE_MAX_OBJECTS];

	GPUBuffer merged_vertex_buffer;
	GPUBuffer merged_index_buffer;
	
	u32 material_count;
	Material materials[SCENE_MAX_OBJECTS];

	GPUBuffer *material_buffer;
	
	u32 light_count;
	Light lights[SCENE_MAX_OBJECTS];
	
	// TODO: Move image view cache into here when adding
	//       automatic mip-mapping into the render graph.
} RenderState;

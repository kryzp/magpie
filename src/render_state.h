
typedef struct GPU_FrameData {
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
} GPU_FrameData;

typedef struct GPU_ObjectData {
	m4 model_matrix;
	m4 normal_matrix;
} GPU_ObjectData;

typedef struct GPU_Light {
	v4 position;    // xyz: position,    w: n/a
	v4 colour;      // xyz: colour,      w: intensity
	v4 attenuation; // xyz: attenuation, w: radius
	m4 transform;
} GPU_Light;

typedef struct GPU_Material {
	u32 diffuse_texture;
	u32 normal_texture;
	u32 emissive_texture;
	u32 metallic_roughness_texture;
	u32 ambient_texture;
} GPU_Material;

typedef struct GPU_Instance {
	u32 object_id;
	u32 batch_id;
} GPU_Instance;

typedef struct GPU_Indirect {
	VkDrawIndexedIndirectCommand command;
	u32 batch_id;
} GPU_Indirect;

// ---

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

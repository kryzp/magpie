
typedef struct GPU_ModelVertex {
	v3 position;
	v2 texcoord;
	v3 colour;
	v3 normal;
	v3 tangent;
	v3 bitangent;
} GPU_ModelVertex;

typedef struct GPU_FrameData {
	m4 view;
	m4 projection;
	m4 view_projection;
	m4 view_projection_no_translation;
	m4 inv_view;
	m4 inv_projection;
	v3 camera_position;
	v2 window_resolution;
	f32 time;
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
} GPU_Indirect;

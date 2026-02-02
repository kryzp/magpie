
struct FrameData {
	float4x4 view;
	float4x4 projection;
	float4x4 view_projection;
	float4x4 view_projection_no_translation;
	float4x4 inv_view;
	float4x4 inv_projection;
	float3 camera_position;
	float2 window_resolution;
	float time;
};

struct ModelVertex {
	float3 position;
	float2 texcoord;
	float3 colour;
	float3 normal;
	float3 tangent;
	float3 bitangent;
};

struct ObjectData {
	float4x4 model_matrix;
    float4x4 normal_matrix;
    float4 sphere_bounds;
	uint material_index;
    uint page_index;
    uint mesh_index;
};

struct RenderMesh {
    uint index_count;
    uint first_index;
    ModelVertex *vertex_buffer;
};

struct Material {
    uint albedo_texture;
    uint normal_texture;
    uint emissive_texture;
    uint mr_texture;
    uint ambient_texture;
};

struct Light {
	float3 position;
    float3 colour;
    float intensity;
	float3 attenuation;
	float radius;
	float4x4 transform;
};

struct IndirectDraw {
	uint index_count;
	uint instance_count;
	uint first_index;
	uint vertex_offset;
	uint first_instance;
};

struct PagePointers {
    ModelVertex *vertex_buffer;
    IndirectDraw *opaque_indirect_buffer;
    uint *opaque_count_buffer;
};

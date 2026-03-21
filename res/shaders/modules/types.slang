module types;

namespace types
{
	public struct FrameData {
		public float4x4 view;
		public float4x4 projection;
		public float4x4 view_projection;
		public float4x4 view_projection_no_translation;
		public float4x4 inv_view;
		public float4x4 inv_projection;
		public float3 camera_position;
		public float2 window_resolution;
		public float time;
	};

	public struct ModelVertex {
		public float3 position;
		public float2 texcoord;
		public float3 colour;
		public float3 normal;
		public float3 tangent;
		public float3 bitangent;
	};

	public struct ObjectData {
		public float4x4 model_matrix;
		public float4x4 normal_matrix;
		public float4 sphere_bounds;
		public uint material_index;
		public uint page_index;
		public uint mesh_index;
	};

	public struct RenderMesh {
		public uint index_count;
		public uint first_index;
		public ModelVertex *vertex_buffer;
	};

	public struct Material {
		public uint albedo_texture;
		public uint normal_texture;
		public uint emissive_texture;
		public uint mr_texture;
		public uint ambient_texture;
	};

	public struct Light {
		public float4x4 transform;
		public float3 position;
		public float radius;
		public float3 colour;
		public float intensity;
		public float3 attenuation;
		public int shadow_slot_index; // -1 to disable shadows
	};

	public struct ShadowCaster {
		public float4x4 face_matrices[6];
		public float3 position;
		public float near_plane;
		public float far_plane;
		public int shadow_map;
	};

	public struct IndirectDraw {
		public uint index_count;
		public uint instance_count;
		public uint first_index;
		public uint vertex_offset;
		public uint first_instance;
	};

	public struct PagePointers {
		public ModelVertex *vertex_buffer;
	};
}

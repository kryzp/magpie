module scene;

namespace scene
{
	public struct ModelVertex
	{
		public float3 position;
		public float2 texcoord;
		public float3 colour;
		public float3 normal;
		public float3 tangent;
		public float3 bitangent;
	};

	public struct ObjectData
	{
		public float4x4 model_matrix;
		public float4x4 normal_matrix;
		public float4 sphere_bounds;
		public uint material_index;
		public uint page_index;
		public uint mesh_index;
	};

	public struct RenderMesh
	{
		public uint index_count;
		public uint first_index;
		public ModelVertex *vertex_buffer;
	};

	public struct Material
	{
		public uint albedo_texture;
		public uint normal_texture;
		public uint emissive_texture;
		public uint mr_texture;
		public uint ambient_texture;

		public float4 albedo_factor;
		public float metallic_factor;
		public float roughness_factor;
		public float emissive_factor;

		public uint double_sided;
	};

	public struct IndirectDraw
	{
		public uint index_count;
		public uint instance_count;
		public uint first_index;
		public uint vertex_offset;
		public uint first_instance;
	};

	public struct PagePointers
	{
		public ModelVertex *vertex_buffer;
	};
}

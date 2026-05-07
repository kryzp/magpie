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

	public static const uint ALPHA_MODE_OPAQUE = 0;
	public static const uint ALPHA_MODE_MASK   = 1;
	public static const uint ALPHA_MODE_BLEND  = 2;

	public struct ObjectData
	{
		public float4x4 model_matrix;
		public float4x4 normal_matrix;
									 
		public float4 sphere_bounds;
								   
		public uint material_index;
		public uint page_index;
		public uint mesh_index;
							  
		public uint alpha_mode; // 0 = opaque, 1 = mask, 2 = blend (for culling)
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
		public uint metallic_roughness_texture;
		public uint emissive_texture;
		public uint occlusion_texture;
	
		public float4 albedo_factor;
		public float  normal_scale;
		public float  metallic_factor;
		public float  roughness_factor;
		public float3 emissive_factor;
		public float  emissive_intensity;
		public float  occlusion_intensity;

		// ---

		public float ior;
	
		// ---
	
		public uint transmission_texture;
		public uint thickness_texture;

		public float transmission_factor;
		public float thickness_factor;

		public float3 attenuation_colour;
		public float  attenuation_distance;

		// ---
	
		public uint specular_texture;
		public uint specular_colour_texture;

		public float  specular_factor;
		public float3 specular_colour_factor;

		// ---
	
		public uint clearcoat_texture;
		public uint clearcoat_roughness_texture;

		public float clearcoat_factor;
		public float clearcoat_roughness_factor;

		// ---
	
		public uint sheen_colour_texture;
		public uint sheen_roughness_texture;

		public float3 sheen_colour_factor;
		public float  sheen_roughness_factor;
	
		// ---
	
		public uint iridescence_texture;
		public uint iridescence_thickness_texture;

		public float iridescence_factor;
		public float iridescence_ior;
		public float iridescence_thickness_min_nanometers;
		public float iridescence_thickness_max_nanometers;

		// ---
	
		public uint double_sided;
		public uint unlit;
		public float alpha_cutoff;
		public uint alpha_mode;
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

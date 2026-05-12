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
	 
	public struct SkinVertex
	{
		public uint32_t joints[4];
		public float weights[4];
	};

	public static const uint32_t ALPHA_MODE_OPAQUE = 0;
	public static const uint32_t ALPHA_MODE_MASK   = 1;
	public static const uint32_t ALPHA_MODE_BLEND  = 2;

	public struct ObjectData
	{
		public float4x4 model_matrix;
		public float4x4 normal_matrix;
									 
		public float4 sphere_bounds;
								   
		public uint32_t material_index;
		public uint32_t page_index;
		public uint32_t mesh_index;
							  
		public uint32_t alpha_mode; // 0 = opaque, 1 = mask, 2 = blend (for culling)

		public float4x4 *skinning_palette;
		public uint32_t skinning_joint_count;

		public uint32_t _padding0; // todo: why the fuck do we need padding? i genuinely spent like 3 hours bruteforcing this what the fuck? we use scalar block layout????
	};

	public struct RenderMesh
	{
		public uint32_t index_count;
		public uint32_t first_index;
		public ModelVertex *vertex_buffer;
		public SkinVertex *skin_buffer;
	};

	public float4x4 ComputeSkinningMatrix(SkinVertex v, float4x4 *palette)
	{
		return (v.weights[0] * palette[v.joints[0]] +
				v.weights[1] * palette[v.joints[1]] +
				v.weights[2] * palette[v.joints[2]] +
				v.weights[3] * palette[v.joints[3]]);
	}

	public bool IsSkinned(ObjectData o, RenderMesh m)
	{
		return (o.skinning_palette != nullptr &&
				m.skin_buffer != nullptr);
	}

	public struct Material
	{
		public uint32_t albedo_texture;
		public uint32_t normal_texture;
		public uint32_t metallic_roughness_texture;
		public uint32_t emissive_texture;
		public uint32_t occlusion_texture;
	
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
	
		public uint32_t transmission_texture;
		public uint32_t thickness_texture;

		public float transmission_factor;
		public float thickness_factor;

		public float3 attenuation_colour;
		public float  attenuation_distance;

		// ---
	
		public uint32_t specular_texture;
		public uint32_t specular_colour_texture;

		public float  specular_factor;
		public float3 specular_colour_factor;

		// ---
	
		public uint32_t clearcoat_texture;
		public uint32_t clearcoat_roughness_texture;

		public float clearcoat_factor;
		public float clearcoat_roughness_factor;

		// ---
	
		public uint32_t sheen_colour_texture;
		public uint32_t sheen_roughness_texture;

		public float3 sheen_colour_factor;
		public float  sheen_roughness_factor;
	
		// ---
	
		public uint32_t iridescence_texture;
		public uint32_t iridescence_thickness_texture;

		public float iridescence_factor;
		public float iridescence_ior;
		public float iridescence_thickness_min_nanometers;
		public float iridescence_thickness_max_nanometers;

		// ---
	
		public uint32_t double_sided;
		public uint32_t unlit;
		public float alpha_cutoff;
		public uint32_t alpha_mode;
	};

	public struct IndirectDraw
	{
		public uint32_t index_count;
		public uint32_t instance_count;
		public uint32_t first_index;
		public uint32_t vertex_offset;
		public uint32_t first_instance;
	};

	public struct PagePointers
	{
		public ModelVertex *vertex_buffer;
	};
}

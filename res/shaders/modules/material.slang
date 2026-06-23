module material;

import bindless;
import scene;
import lighting;
import frame;

namespace material
{
	public struct SurfaceInput
	{
		public float3 world_position;
		public float2 texcoord;
		public float3 vertex_colour;
		public float3x3 tbn;
	};

	public lighting::Surface SampleSurface(SurfaceInput input, scene::Material mat, frame::FrameData frame_data, uint sampler_idx)
	{
		SamplerState sampler = bdl::samplers[sampler_idx];
		float2 uv = frac(input.texcoord);

		
		float4 albedo = mat.albedo_factor * float4(input.vertex_colour, 1.0);

		if (bdl::IsValid(mat.albedo_texture))
			albedo *= bdl::textures[mat.albedo_texture].Sample(sampler, uv).rgba;


		// glTF standard is that:
		// r = unused(?)
		// g = roughness
		// b = metallic
		// TODO: it should be called roughness_metallic_texture!!!!!!!!!!!!
		float metallic = mat.metallic_factor;
		float roughness = mat.roughness_factor;

		if (bdl::IsValid(mat.metallic_roughness_texture))
		{
			float3 mr = bdl::textures[mat.metallic_roughness_texture].Sample(sampler, uv).rgb;

			roughness *= mr.g;
			metallic *= mr.b;
		}


		float3 normal = float3(0.0, 0.0, 1.0);

		if (bdl::IsValid(mat.normal_texture))
		{
			normal = bdl::textures[mat.normal_texture].Sample(sampler, uv).rgb;
			normal = 2.0 * normal - 1.0;
			normal.xy *= mat.normal_scale;
		}

		normal = normalize(mul(input.tbn, normal));


		float3 emissive = mat.emissive_factor * mat.emissive_intensity;

		if (bdl::IsValid(mat.emissive_texture))
			emissive *= bdl::textures[mat.emissive_texture].Sample(sampler, uv).rgb;


		float occlusion = 1.0;

		if (bdl::IsValid(mat.occlusion_texture))
			occlusion = lerp(1.0, bdl::textures[mat.occlusion_texture].Sample(sampler, uv).r, mat.occlusion_intensity);


		float specular_strength = mat.specular_factor;

		if (bdl::IsValid(mat.specular_texture))
			specular_strength *= bdl::textures[mat.specular_texture].Sample(sampler, uv).a;
		
		float3 specular_colour = mat.specular_colour_factor;

		if (bdl::IsValid(mat.specular_colour_texture))
			specular_colour *= bdl::textures[mat.specular_colour_texture].Sample(sampler, uv).rgb;


		// glTF spec ((ior-1)/(ior+1))^2
		float F0_dialectric = pow((mat.ior - 1.0) / (mat.ior + 1.0), 2.0);
		float3 F0_dialectric_specular = min(F0_dialectric * specular_colour, 1.0) * specular_strength;
		float3 F0 = lerp(F0_dialectric_specular, albedo.rgb, metallic);


		lighting::Surface s;

		
		s.world_position = input.world_position;
		s.world_normal = normal;
		s.view_dir = normalize(frame_data.camera_position.xyz - input.world_position);
		s.NdotV = max(0.0, dot(s.world_normal, s.view_dir));

		
		s.albedo = albedo.rgb;
		s.alpha = albedo.a;
		s.F0 = F0;
		s.roughness = roughness;
		s.metallic = metallic;
		s.occlusion = occlusion;
		s.emissive = emissive;

		
		s.ior = mat.ior;

		
		s.specular = mat.specular_factor;
		s.specular_colour = mat.specular_colour_factor;

		
		s.transmission = mat.transmission_factor;

		if (bdl::IsValid(mat.transmission_texture))
			s.transmission *= bdl::textures[mat.transmission_texture].Sample(sampler, uv).r;

		s.thickness = mat.thickness_factor;

		if (bdl::IsValid(mat.thickness_texture))
			s.thickness *= bdl::textures[mat.thickness_texture].Sample(sampler, uv).g;

		s.attenuation_colour = mat.attenuation_colour;
		s.attenuation_distance = mat.attenuation_distance;


		s.clearcoat = mat.clearcoat_factor;

		if (bdl::IsValid(mat.clearcoat_texture))
			s.clearcoat *= bdl::textures[mat.clearcoat_texture].Sample(sampler, uv).r;

		s.clearcoat_roughness = mat.clearcoat_roughness_factor;

		if (bdl::IsValid(mat.clearcoat_roughness_texture))
			s.clearcoat_roughness *= bdl::textures[mat.clearcoat_roughness_texture].Sample(sampler, uv).g;

		s.clearcoat_normal = s.world_normal; // TODO: should have a seperate normal map oops.
		s.clearcoat_NdotV = s.NdotV;
		

		s.sheen_colour = mat.sheen_colour_factor;

		if (bdl::IsValid(mat.sheen_colour_texture))
			s.sheen_colour *= bdl::textures[mat.sheen_colour_texture].Sample(sampler, uv).rgb;
		
		s.sheen_roughness = mat.sheen_roughness_factor;

		if (bdl::IsValid(mat.sheen_colour_texture))
			s.sheen_roughness *= bdl::textures[mat.sheen_roughness_texture].Sample(sampler, uv).a;

		
		s.iridescence = mat.iridescence_factor;

		if (bdl::IsValid(mat.iridescence_texture))
			s.iridescence *= bdl::textures[mat.iridescence_texture].Sample(sampler, uv).r;

		s.iridescence_ior = mat.iridescence_ior;
		s.iridescence_thickness = mat.iridescence_thickness_max_nanometers;

		if (bdl::IsValid(mat.iridescence_thickness_texture))
		{
			float t = bdl::textures[mat.iridescence_thickness_texture].Sample(sampler, uv).g;

			s.iridescence_thickness = lerp(mat.iridescence_thickness_min_nanometers,
										   mat.iridescence_thickness_max_nanometers,
										   t);
		}

		
		return s;
	}

	public bool ShouldDiscard(float alpha, uint alpha_mode, float cutoff)
	{
		return (alpha_mode == scene::ALPHA_MODE_MASK) && (alpha < cutoff);
	}
}

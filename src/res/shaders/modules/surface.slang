module surface;

import bindless;
import scene;
import frame;
import math;

namespace surface {

public struct SurfaceInput {
	public float3 world_position;
	public float2 texcoord;
	public float3 vertex_colour;
	public float3x3 tbn;
};

/*
 * Contains all info required to evaluate shading at a point.
 */
public struct Surface {

	// Geometry
	public float3 world_position;
	public float3 world_normal;
	public float3 geometric_normal;
	public float3 view_dir; // point -> camera
	public float NdotV;

	// PBR
	public float3 albedo;
	public float  roughness;
	public float  metallic;
	public float  occlusion;
	public float3 emissive;
	public float  alpha;
	public float3 F0;

	public float  ior;

	public float  transmission;
	public float  thickness;

	public float3 attenuation_colour;
	public float  attenuation_distance;

	public float  specular;
	public float3 specular_colour;

	public float  clearcoat;
	public float  clearcoat_roughness;
	public float3 clearcoat_normal;
	public float  clearcoat_NdotV;

	public float3 sheen_colour;
	public float  sheen_roughness;

	public float  iridescence;
	public float  iridescence_ior;
	public float  iridescence_thickness;
};

// TODO: BINDLESSISVALID CHECKS ARE SLOW AS FUCK, REPLACE WITH JUST A DEFAULT WHITE PIXEL OR SOMETHING

public Surface SampleSurface(SurfaceInput input, scene::Material mat, frame::FrameData frame_data, uint sampler)
{
	float2 uv = input.texcoord * float2(1.0, -1.0); // temporarily flipped so that the ktx2 bistro looks good but this is a shit solution


	float4 albedo = mat.albedo_factor * float4(input.vertex_colour, 1.0);

	if (BindlessIsValid(mat.albedo_texture)) {
		albedo *= SampleBindless(mat.albedo_texture, sampler, uv).rgba;
	}

	// glTF standard is that:
	// r = unused(?)
	// g = roughness
	// b = metallic
	// TODO: it should be called roughness_metallic_texture!!!!!!!!!!!!
	float metallic = mat.metallic_factor;
	float roughness = mat.roughness_factor;

	if (BindlessIsValid(mat.metallic_roughness_texture)) {
		float3 mr = SampleBindless(mat.metallic_roughness_texture, sampler, uv).rgb;
		roughness *= mr.g;
		metallic *= mr.b;
	}


	float3 normal = float3(0.0, 0.0, 1.0);

	if (BindlessIsValid(mat.normal_texture)) {
		normal = SampleBindless(mat.normal_texture, sampler, uv).rgb;
		normal = 2.0 * normal - 1.0;
		normal.xy *= mat.normal_scale;
	}

	normal = normalize(mul(input.tbn, normal));


	float3 emissive = mat.emissive_factor * mat.emissive_intensity;

	if (BindlessIsValid(mat.emissive_texture)) {
		emissive *= SampleBindless(mat.emissive_texture, sampler, uv).rgb;
	}

	float occlusion = 1.0;

	if (BindlessIsValid(mat.occlusion_texture)) {
		occlusion = lerp(1.0, SampleBindless(mat.occlusion_texture, sampler, uv).r, mat.occlusion_intensity);
	}

	float specular_strength = mat.specular_factor;

	if (BindlessIsValid(mat.specular_texture)) {
		specular_strength *= SampleBindless(mat.specular_texture, sampler, uv).a;
	}

	float3 specular_colour = mat.specular_colour_factor;

	if (BindlessIsValid(mat.specular_colour_texture)) {
		float4 sc = SampleBindless(mat.specular_colour_texture, sampler, uv);
		specular_colour *= sc.rgb;

		// in this case alpha is glossiness
		// glossiness = 1.0 - roughness
		if (mat.gloss_in_specular_alpha != 0)
			roughness = 1.0 - (1.0 - mat.roughness_factor) * sc.a;
	}

	// glTF spec ((ior-1)/(ior+1))^2
	float F0_dialectric = pow((mat.ior - 1.0) / (mat.ior + 1.0), 2.0);
	float3 F0_dialectric_specular = min(F0_dialectric * specular_colour, 1.0) * specular_strength;
	float3 F0 = lerp(F0_dialectric_specular, albedo.rgb, metallic);


	Surface s;

	s.world_position = input.world_position;
	s.world_normal = normal;
	s.geometric_normal = math::SafeNormalize3(mul(input.tbn, float3(0.0, 0.0, 1.0)));
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

	if (BindlessIsValid(mat.transmission_texture)) {
		s.transmission *= SampleBindless(mat.transmission_texture, sampler, uv).r;
	}

	s.thickness = mat.thickness_factor;

	if (BindlessIsValid(mat.thickness_texture)) {
		s.thickness *= SampleBindless(mat.thickness_texture, sampler, uv).r;
	}

	s.attenuation_colour = mat.attenuation_colour;
	s.attenuation_distance = mat.attenuation_distance;


	s.clearcoat = mat.clearcoat_factor;

	if (BindlessIsValid(mat.clearcoat_texture)) {
		s.clearcoat *= SampleBindless(mat.clearcoat_texture, sampler, uv).r;
	}

	s.clearcoat_roughness = mat.clearcoat_roughness_factor;

	if (BindlessIsValid(mat.clearcoat_roughness_texture)) {
		s.clearcoat_roughness *= SampleBindless(mat.clearcoat_roughness_texture, sampler, uv).g;
	}

	s.clearcoat_normal = s.world_normal; // TODO: should have a seperate normal map oops.
	s.clearcoat_NdotV = s.NdotV;


	s.sheen_colour = mat.sheen_colour_factor;

	if (BindlessIsValid(mat.sheen_colour_texture)) {
		s.sheen_colour *= SampleBindless(mat.sheen_colour_texture, sampler, uv).rgb;
	}

	s.sheen_roughness = mat.sheen_roughness_factor;

	if (BindlessIsValid(mat.sheen_roughness_texture)) {
		s.sheen_roughness *= SampleBindless(mat.sheen_roughness_texture, sampler, uv).a;
	}


	s.iridescence = mat.iridescence_factor;

	if (BindlessIsValid(mat.iridescence_texture)) {
		s.iridescence *= SampleBindless(mat.iridescence_texture, sampler, uv).r;
	}

	s.iridescence_ior = mat.iridescence_ior;
	s.iridescence_thickness = mat.iridescence_thickness_max_nanometers;

	if (BindlessIsValid(mat.iridescence_thickness_texture)) {
		float t = SampleBindless(mat.iridescence_thickness_texture, sampler, uv).g;
		s.iridescence_thickness = lerp(
			mat.iridescence_thickness_min_nanometers,
			mat.iridescence_thickness_max_nanometers,
			t
		);
	}

	return s;
}

}

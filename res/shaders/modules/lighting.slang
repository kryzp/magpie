module lighting;

import math;
import irradiance;
import shadows;
import frame;
import bindless;

namespace lighting
{
	public struct Light
	{
		public float4x4 transform;
		public float3 position;
		public float radius;
		public float3 colour;
		public float intensity;
		public float3 attenuation;
		public int shadow_slot_index; // -1 to disable shadows
	};

	/*
	 * Contains all info required to evaluate shading at a point.
	 */
	public struct Surface
	{
		// Geometry
		public float3 world_position;
		public float3 world_normal;
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

		public float3 sheen_colour;
		public float  sheen_roughness;

		public float  iridescence;
		public float  iridescence_ior;
		public float  iridescence_thickness;
	};

	public struct IBL
	{
		public uint irradiance_fallback_cubemap;
		public uint prefilter_cubemap;
		public uint brdf_lut;
		
		public irradiance::ProbeSH       *irr_sh;
		public irradiance::ProbeGridInfo *irr_grid_info;

		public uint sampler;
	};

	// Standard dielectric F0 mix.
	public float3
	ComputeF0(float3 albedo, float metallic)
	{
		return lerp(0.04, albedo, metallic);
	}

	// params = (k_quad, k_lin, k_const).
	public float
	CalculateAttenuation(float distance_squared, float3 params)
	{
		return 1.0 / dot(float3(distance_squared, sqrt(distance_squared), 1.0), params);
	}

	public float3
	FresnelSchlick(float VdotH, float3 F0, float roughness)
	{
		float t = pow(1.0 - max(VdotH, 0.0), 5.0);
		float3 F90 = max(1.0 - roughness, F0);
		return lerp(F0, F90, t);
	}

	public float
	DistributionGGX(float NdotH, float roughness)
	{
		float a = roughness * roughness;
		float a2 = a * a;

		float d = max(NdotH, 0.0);
		float d2 = d * d;

		float denom = d2 * (a2 - 1.0) + 1.0;

		return a2 / (denom * denom * math::PI);
	}

	// Actual physically-accurate equation (used for BRDF LUT integration).
	public float
	GeometrySchlickGGX_PHYSACC(float NdotV, float roughness)
	{
		float r = roughness;
		float k = r * r * 0.5;

		return NdotV / (NdotV * (1.0 - k) + k);
	}

	public float
	GeometrySmith_PHYSACC(float NdotV, float NdotL, float roughness)
	{
		float ggx2 = GeometrySchlickGGX_PHYSACC(NdotV, roughness);
		float ggx1 = GeometrySchlickGGX_PHYSACC(NdotL, roughness);

		return ggx1 * ggx2;
	}

	// Modified because this looks better for direct lighting.
	public float
	GeometrySchlickGGX_ARTISTIC(float NdotV, float roughness)
	{
		float r = roughness + 1.0;
		float k = r * r / 8.0;

		return NdotV / (NdotV * (1.0 - k) + k);
	}

	public float
	GeometrySmith_ARTISTIC(float NdotV, float NdotL, float roughness)
	{
		float ggx2 = GeometrySchlickGGX_ARTISTIC(NdotV, roughness);
		float ggx1 = GeometrySchlickGGX_ARTISTIC(NdotL, roughness);

		return ggx1 * ggx2;
	}

	public float3
	EvaluateBRDF_Base(Surface s, float3 L, float NdotL, float NdotH, float VdotH)
	{
		float3 F = FresnelSchlick(VdotH, s.F0, 0.0);
		float NDF = DistributionGGX(NdotH, s.roughness);
		float G = GeometrySmith_ARTISTIC(s.NdotV, NdotL, s.roughness);

		float3 kD = (1.0 - F) * (1.0 - s.metallic);
		float3 diffuse = s.albedo;
		float3 specular = (F * G * NDF) / (4.0 * NdotL * s.NdotV + 0.0001);

		return kD * diffuse + specular;
	}

	public float3
	EvaluateBRDF_ClearCoat(Surface s, float3 L, float NdotL, float NdotH, float VdotH)
	{
		return 0.0; // TODO
	}

	public float3
	EvaluateBRDF_Sheen(Surface s, float3 L, float NdotL, float NdotH, float VdotH)
	{
		return 0.0; // TODO
	}

	public float3
	EvaluateLight_Ambient(Surface s, IBL ibl)
	{
		SamplerState sampler = bdl::samplers[ibl.sampler];

		TextureCube fallback_irradiance_cubemap = bdl::cubes[ibl.irradiance_fallback_cubemap];
		TextureCube prefilter_cubemap           = bdl::cubes[ibl.prefilter_cubemap];

		float3 F = s.F0;
		float3 kD = (1.0 - F) * (1.0 - s.metallic);

		float3 R = normalize(reflect(-s.view_dir, s.world_normal));

		float prefilter_mip_level = s.roughness * irradiance::MAX_REFLECTION_LOD;
		float3 prefiltered_colour = prefilter_cubemap.SampleLevel(sampler, math::WorldToCubemap(R), prefilter_mip_level).rgb;

		float2 env_brdf = bdl::textures[ibl.brdf_lut].Sample(sampler, float2(s.NdotV, s.roughness)).xy;

		float3 specular = prefiltered_colour * (F * env_brdf.x + env_brdf.y);

		float3 irr = 0.0;

		if (ibl.irr_sh)
			irr = irradiance::SampleIrradianceGrid(s.world_position, s.world_normal, *ibl.irr_grid_info, ibl.irr_sh);
		else
			irr = fallback_irradiance_cubemap.Sample(sampler, math::WorldToCubemap(s.world_normal)).rgb;

		float3 diffuse = irr * s.albedo;

		return (kD * diffuse + specular) * s.occlusion;
	}

	public float3
	EvaluateLight_Point(Surface s, Light light, shadows::ShadowCaster *casters, uint shadow_sampler_idx, frame::FrameData frame_data)
	{
		float3 delta_x = light.position - s.world_position;
		float distance_sqr = dot(delta_x, delta_x);

		float attenuation = CalculateAttenuation(distance_sqr, light.attenuation);
		float3 radiance = light.colour * light.intensity * attenuation;

		float3 L = normalize(delta_x);
		float3 H = normalize(L + s.view_dir);

		float NdotL = max(0.0, dot(s.world_normal, L));
		float NdotH = max(0.0, dot(s.world_normal, H));
		float VdotH = max(0.0, dot(H, s.view_dir));

		if (NdotL <= 0.0)
			return 0.0;

		float3 base = EvaluateBRDF_Base(s, L, NdotL, NdotH, VdotH);
		float3 clear_coat = (s.clearcoat > 0.0) ? EvaluateBRDF_ClearCoat(s, L, NdotL, NdotH, VdotH) : 0.0;
		float3 sheen = (length(s.sheen_colour) > 0.0) ? EvaluateBRDF_Sheen(s, L, NdotL, NdotH, VdotH) : 0.0;

		float shadow_factor = 1.0;

		if (light.shadow_slot_index >= 0)
		{
			shadows::ShadowCaster caster = casters[light.shadow_slot_index];
			shadow_factor = shadows::CalculatePointShadow(s.world_position, light.position, caster, frame_data, shadow_sampler_idx);
		}

		return shadow_factor * radiance * NdotL * (base + clear_coat + sheen);
	}
}

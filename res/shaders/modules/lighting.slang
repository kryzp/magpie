module lighting;
import math;

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

	// Standard dielectric F0 mix.
	public float3 ComputeF0(float3 albedo, float metallic)
	{
		return lerp(0.04, albedo, metallic);
	}

	// params = (k_quad, k_lin, k_const).
	public float CalculateAttenuation(float distance_squared, float3 params)
	{
		return 1.0 / dot(float3(distance_squared, sqrt(distance_squared), 1.0), params);
	}

	public float3 FresnelSchlick(float VdotH, float3 F0, float roughness)
	{
		float t = pow(1.0 - max(VdotH, 0.0), 5.0);
		float3 F90 = max(1.0 - roughness, F0);
		return lerp(F0, F90, t);
	}

	public float DistributionGGX(float NdotH, float roughness)
	{
		float a = roughness * roughness;
		float a2 = a * a;

		float d = max(NdotH, 0.0);
		float d2 = d * d;

		float denom = d2 * (a2 - 1.0) + 1.0;

		return a2 / (denom * denom * math::PI);
	}

	// Actual physically-accurate equation (used for BRDF LUT integration).
	public float GeometrySchlickGGX_PHYSACC(float NdotV, float roughness)
	{
		float r = roughness;
		float k = r * r * 0.5;

		return NdotV / (NdotV * (1.0 - k) + k);
	}

	public float GeometrySmith_PHYSACC(float NdotV, float NdotL, float roughness)
	{
		float ggx2 = GeometrySchlickGGX_PHYSACC(NdotV, roughness);
		float ggx1 = GeometrySchlickGGX_PHYSACC(NdotL, roughness);

		return ggx1 * ggx2;
	}

	// Modified because this looks better for direct lighting.
	public float GeometrySchlickGGX_ARTISTIC(float NdotV, float roughness)
	{
		float r = roughness + 1.0;
		float k = r * r / 8.0;

		return NdotV / (NdotV * (1.0 - k) + k);
	}

	public float GeometrySmith_ARTISTIC(float NdotV, float NdotL, float roughness)
	{
		float ggx2 = GeometrySchlickGGX_ARTISTIC(NdotV, roughness);
		float ggx1 = GeometrySchlickGGX_ARTISTIC(NdotL, roughness);

		return ggx1 * ggx2;
	}
}

module util;

namespace util
{
	public const static float PI = 3.14159265;

	// We use a right-handed Z-up world
	// coordinate system, but cubemap
	// sampling requires Y-up left handed.
	// Thanks RenderMan!!
	// Warning: Does NOT normalize.
	public float3 WorldToCubemap(float3 v)
	{
		return float3(v.x, v.z, v.y);
	}

	public float RadicalInverse_VdC(uint bits) 
	{
		bits = (bits << 16u) | (bits >> 16u);
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return float(bits) * 2.3283064365386963e-10;
	}

	public float2 Hammersley(uint i, uint N)
	{
		return float2(float(i) / float(N), RadicalInverse_VdC(i));
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
	
		return a2 / (denom * denom * util::PI);
	}

	// Actual physically-accurate equation.
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

	// Modified because this looks better for lighting.
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

	public void BuildOrthonormalBasis(float3 normal, out float3 tangent, out float3 bitangent)
	{
		float3 u = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);

		tangent = normalize(cross(normal, u));
		bitangent = normalize(cross(tangent, normal));

		/*
		float s = normal.z >= 0.0 ? 1.0 : -1.0;

		float a = -1.0 / (s + normal.z);
		float b = normal.x * normal.y * a;

		tangent = float3(
			1.0 + (s * normal.x * normal.x * a),
			s * b,
			-s * normal.x
		);

		bitangent = float3(
			b,
			s + (normal.y * normal.y * a),
			-normal.y
		);
		*/
	}

	public float3 ImportanceSampleGGX(float2 Xi, float3 normal, float roughness)
	{
		float a = roughness * roughness;
	
		float phi = 2.0 * util::PI * Xi.x;
	
		float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
		float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

		float3 H = float3(
			sin_theta * cos(phi),
			sin_theta * sin(phi),
			cos_theta
		);

		float3 tangent, bitangent;
		BuildOrthonormalBasis(normal, tangent, bitangent);

		float3x3 tbn = transpose(float3x3(tangent, bitangent, normal));
		float3 sample_vector = mul(tbn, H);
	
		return normalize(sample_vector);
	}
}

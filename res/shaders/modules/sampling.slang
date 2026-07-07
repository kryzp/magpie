module sampling;

import math;

namespace sampling
{
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

	public float3 ImportanceSampleGGX(float2 Xi, float3 normal, float roughness)
	{
		float a = roughness * roughness;

		float phi = 2.0 * math::PI * Xi.x;

		float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
		float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

		float3 H = float3(sin_theta * cos(phi),
						  sin_theta * sin(phi),
						  cos_theta);

		float3 tangent, bitangent;
		math::BuildOrthonormalBasis(normal, tangent, bitangent);

		float3x3 tbn = transpose(float3x3(tangent, bitangent, normal));
		float3 sample_vector = mul(tbn, H);

		return normalize(sample_vector);
	}
}

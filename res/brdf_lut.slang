#include "util.slang"

const static uint SAMPLE_COUNT = 1024u;

struct VS_Output {
	float4 sv_position : SV_Position;
	[vk::location(0)] float2 uv;
};

[shader("vertex")]
VS_Output VertexMain(uint id : SV_VertexID)
{
	VS_Output output;
	output.uv = float2((id << 1) & 2, id & 2);
	output.sv_position = float4(output.uv.x*2. - 1., 1. - 2.*output.uv.y, 0., 1.);

	return output;
}

// The Geometry-Schlick GGX function is slightly different
// for BRDF since we compute k differently for better precision.
// (I think??)
float GeometrySchlickGGX_BRDF_LUT(float NdotV, float roughness)
{
	float r = roughness;
	float k = r * r * 0.5;
	
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith_BRDF_LUT(float NdotV, float NdotL, float roughness)
{
	float ggx2 = GeometrySchlickGGX_BRDF_LUT(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX_BRDF_LUT(NdotL, roughness);
	
	return ggx1 * ggx2;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	float NdotV = input.uv.x;
	float roughness = max(input.uv.y, 0.04);

	float3 V = float3(
		sqrt(1.0 - NdotV * NdotV),
		0.0,
		NdotV
	);

	float A = 0.0;
	float B = 0.0;

	float3 N = float3(0.0, 0.0, 1.0);

	for (uint i = 0u; i < SAMPLE_COUNT; i++) {
		// Generates a sample vector that's biased towards the
		// preferred alignment direction (importance sampling).

		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(0.0, L.z);
		float NdotH = max(0.0, H.z);
		float VdotH = max(0.0, dot(V, H));

		if (NdotL > 0.0) {
			float G = GeometrySmith_BRDF_LUT(NdotV, NdotL, roughness);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5.0);

			A += G_Vis * (1.0 - Fc);
			B += G_Vis * Fc;
		}
	}

	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);

	return float4(A, B, 0.0, 1.0);
}

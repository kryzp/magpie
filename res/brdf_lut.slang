#include "util.slang"

const static int SAMPLE_COUNT = 1024;

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

// NOTE(kp): The Geometry-Schlick GGX function is slightly different
//           for BRDF since we compute k differently for better precision.
//           (I think??)
float GeometrySchlickGGX_BRDF_LUT(float NdotV, float roughness)
{
	float r = roughness;
	float k = r * r * 0.5;
	
	return NdotV / lerp(k, 1., NdotV);
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
	float roughness = input.uv.y;

	float3 V = float3(sqrt(1. - NdotV*NdotV),
			  0.,
			  NdotV);

	float A = 0.;
	float B = 0.;

	float3 N = float3(0., 0., 1.);

	for (int i = 0; i < SAMPLE_COUNT; ++i) {
		// NOTE(kp): Generates a sample vector that's biased towards the
		//           preferred alignment direction (importance sampling).

		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2. * dot(V, H) * H - V);

		float NdotL = max(0., dot(N, L));

		if (NdotL > 0.) {
			float NdotH = max(0., H.z);
			float VdotH = max(0., dot(V, H));

			float G = GeometrySmith_BRDF_LUT(NdotV, NdotL, roughness);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1. - VdotH, 5.);

			A += (1. - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}

	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);

	return float4(A, B, 0., 1.);
}

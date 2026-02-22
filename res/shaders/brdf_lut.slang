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
    output.uv = float2(id & 2, (id << 1) & 2);
    output.sv_position = float4(output.uv.x*2.0 - 1.0, 1.0 - 2.0*output.uv.y, 0.0, 1.0);

	return output;
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

		float NdotL = max(0.0, dot(N, L));
		float NdotH = max(0.0, dot(N, H));
		float VdotH = max(0.0, dot(V, H));

		if (NdotL > 0.0) {
			float G = GeometrySmith_PHYSACC(NdotV, NdotL, roughness);
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

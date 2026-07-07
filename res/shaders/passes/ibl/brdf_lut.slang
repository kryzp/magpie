import sampling;
import lighting;
import fullscreen;

const static uint SAMPLE_COUNT = 1024u;

[shader("vertex")]
fullscreen::VS_Output VertexMain(uint id : SV_VertexID)
{
	return fullscreen::FullscreenVS(id);
}

[shader("fragment")]
float4 FragmentMain(fullscreen::VS_Output input) : SV_Target
{
	float NdotV = input.uv.x;
	float roughness = max(input.uv.y, 0.04);

	float3 V = float3(sqrt(1.0 - NdotV * NdotV),
					  0.0,
					  NdotV);

	float A = 0.0;
	float B = 0.0;

	float3 N = float3(0.0, 0.0, 1.0);

	for (uint i = 0u; i < SAMPLE_COUNT; i++)
	{
		// Generates a sample vector that's biased towards the
		// preferred alignment direction (importance sampling).

		float2 Xi = sampling::Hammersley(i, SAMPLE_COUNT);
		float3 H = sampling::ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(0.0, dot(N, L));
		float NdotH = max(0.0, dot(N, H));
		float VdotH = max(0.0, dot(V, H));

		if (NdotL > 0.0)
		{
			float G = lighting::GeometrySmith_PHYSACC(NdotV, NdotL, roughness);
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

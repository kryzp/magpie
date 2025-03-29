#include "common.hlsl"

struct PushConstants
{
	float exposure;
	float bloomIntensity;
};

PUSH_CONSTANTS(PushConstants, pc);

struct PSInput
{
	[[vk::location(0)]]
	float2 uv : TEXCOORD0;
};

Texture2D renderTarget : register(t0);
SamplerState samplerState : register(s0);

Texture2D bloomTexture : register(t1);
SamplerState bloomSampler : register(s1);

float4 main(PSInput input) : SV_TARGET
{
	float3 col = renderTarget.Sample(samplerState, input.uv).rgb;
	float3 bloom = bloomTexture.Sample(bloomSampler, input.uv).rgb;
	
	col = lerp(col, bloom, pc.bloomIntensity);
	
	col = 1.0 - exp(-col * pc.exposure);
	
	return float4(col, 1.0);
}

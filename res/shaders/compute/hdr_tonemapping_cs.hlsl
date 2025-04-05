
struct PushConstants
{
	uint width;
	uint height;
	float exposure;
//	float blurIntensity;
};

[[vk::push_constant]]
PushConstants pc;

RWTexture2D<float4> target : register(u0);
//Texture2D<float4> bloom : register(t1);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThread : SV_DispatchThreadID)
{
	if (dispatchThread.x < 0 ||
		dispatchThread.x >= pc.width ||
		dispatchThread.y < 0 ||
		dispatchThread.y >= pc.height)
	{
		return;
	}

	float3 colour = target[dispatchThread.xy].rgb;
	
	colour = 1.0 - exp(-colour * pc.exposure);
	
	target[dispatchThread.xy] = float4(colour, 1.0);
}

#if 0
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

float4 main(PSInput input) : SV_Target
{
	float3 col = renderTarget.Sample(samplerState, input.uv).rgb;
	float3 bloom = bloomTexture.Sample(bloomSampler, input.uv).rgb;
	
	col = lerp(col, bloom, pc.bloomIntensity);
	
	col = 1.0 - exp(-col * pc.exposure);
	
	return float4(col, 1.0);
}
#endif

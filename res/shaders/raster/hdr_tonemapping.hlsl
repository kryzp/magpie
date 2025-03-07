struct PSInput
{
	[[vk::location(0)]]
	float2 uv : TEXCOORD0;
};

#define GAMMA (1.0 / 2.2)

Texture2D renderTarget : register(t0);
SamplerState samplerState : register(s0);

float4 main(PSInput input) : SV_TARGET
{
	float3 col = renderTarget.Sample(samplerState, input.uv).rgb;
	
	col = 1.0 - exp(-col * 2.0);
	
	return float4(col, 1.0);
}

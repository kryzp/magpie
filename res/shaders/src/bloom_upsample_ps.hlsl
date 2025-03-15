struct PushConstants
{
	float filterRadius;
};

[[vk::push_constant]]
PushConstants pc;

struct PSInput
{
	[[vk::location(0)]]
	float2 texCoord : TEXCOORD0;
};

Texture2D renderTarget : register(t0);
SamplerState samplerState : register(s0);

float4 main(PSInput input) : SV_TARGET
{
	float dx = pc.filterRadius;
	float dy = pc.filterRadius;
	
	float2 uv = input.texCoord;
	
	float3 a = renderTarget.Sample(samplerState, float2(uv.x - dx,	uv.y + dy	)).rgb;
	float3 b = renderTarget.Sample(samplerState, float2(uv.x,		uv.y + dy	)).rgb;
	float3 c = renderTarget.Sample(samplerState, float2(uv.x + dx,	uv.y + dy	)).rgb;
	float3 d = renderTarget.Sample(samplerState, float2(uv.x - dx,	uv.y		)).rgb;
	float3 e = renderTarget.Sample(samplerState, float2(uv.x,		uv.y		)).rgb;
	float3 f = renderTarget.Sample(samplerState, float2(uv.x + dx,	uv.y		)).rgb;
	float3 g = renderTarget.Sample(samplerState, float2(uv.x - dx,	uv.y - dy	)).rgb;
	float3 h = renderTarget.Sample(samplerState, float2(uv.x,		uv.y - dy	)).rgb;
	float3 i = renderTarget.Sample(samplerState, float2(uv.x + dx,	uv.y - dy	)).rgb;
	
	float3 upsample = e * 4.0;
	
	upsample += (b + d + f + h) * 2.0;
	upsample += (a + c + g + i);
	
	upsample /= 16.0;
	
	return float4(upsample, 1.0);
}


struct PSInput
{
	[[vk::location(0)]] float2 uv : TEXCOORD0;
};

Texture2D texture : register(t0);
SamplerState samplerState : register(s0);

float4 main(PSInput input) : SV_TARGET
{
	return texture.Sample(samplerState, input.uv);
}

struct PSInput
{
	[[vk::location(0)]]
	float2 texCoord : TEXCOORD0;
};

Texture2D texture : register(t0);
SamplerState samplerState : register(s0);

float4 main(PSInput input) : SV_Target
{
	return texture.Sample(samplerState, input.texCoord);
}

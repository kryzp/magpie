
struct PSInput
{
    [[vk::location(0)]]
    float3 position : TEXCOORD1;
};

TextureCube skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float3 dir = normalize(input.position);
	return float4(skyboxTexture.Sample(skyboxSampler, dir).rgb, 1.0);
}

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
    dir.z *= -1.0;
    
	float3 col = skyboxTexture.Sample(skyboxSampler, dir).rgb;
	return float4(col, 1.0);
}

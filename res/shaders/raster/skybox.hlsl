#include "common_ps.hlsl"

struct PSInput
{
    [[vk::location(VS_OUT_SLOT_POSITION)]]
    float3 position : TEXCOORD1;
    
    [[vk::location(VS_OUT_SLOT_COLOUR)]]
    float3 colour : COLOR;
};

TextureCube localIrradianceMap : register(t3);
SamplerState localIrradianceMapSampler : register(s3);

TextureCube localPrefilterMap : register(t4);
SamplerState localPrefilterMapSampler : register(s4);

Texture2D brdfLUT : register(t5);
SamplerState brdfLUTSampler : register(s5);

TextureCube skyboxTexture : register(t6);
SamplerState skyboxSampler : register(s6);

float4 main(PSInput input) : SV_TARGET
{
    float3 dir = normalize(input.position);
	float4 col = skyboxTexture.Sample(skyboxSampler, dir);
    return col * float4(input.colour, 1.0);
}

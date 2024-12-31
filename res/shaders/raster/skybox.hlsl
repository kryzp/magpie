#include "common_ps.hlsl"

struct PSInput
{
    [[vk::location(VS_OUT_SLOT_POSITION)]] float3 position : TEXCOORD1;
    [[vk::location(VS_OUT_SLOT_COLOUR)]] float3 colour : COLOR;
};

TextureCube skyboxTexture : register(t3);
SamplerState samplerState : register(s3);

float4 main(PSInput input) : SV_TARGET
{
    float3 dir = normalize(input.position);
    float4 col = skyboxTexture.Sample(samplerState, dir);
    return col * float4(input.colour, 1.0);
}

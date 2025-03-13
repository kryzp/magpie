#include "common_vs.hlsl"

struct VSInput
{
    [[vk::location(VS_ATT_SLOT_POSITION)]]
    float3 position : POSITION;
    
    [[vk::location(VS_ATT_SLOT_UV)]]
    float2 uv : TEXCOORD0;
    
    [[vk::location(VS_ATT_SLOT_COLOUR)]]
    float3 colour : COLOR;
    
    [[vk::location(VS_ATT_SLOT_NORMAL)]]
    float3 normal : NORMAL;
    
    [[vk::location(VS_ATT_SLOT_TANGENT)]]
    float3 tangent : TANGENT;
};

struct VSOutput
{
    float4 svPosition : SV_Position;

    [[vk::location(VS_OUT_SLOT_POSITION)]]
    float3 position : TEXCOORD0;
    
    [[vk::location(VS_OUT_SLOT_UV)]]
    float2 texCoord : TEXCOORD1;
    
    [[vk::location(VS_OUT_SLOT_COLOUR)]]
    float3 colour : COLOR;
    
    [[vk::location(VS_OUT_SLOT_TANGENT_FRAG_POS)]]
    float3 fragPos : TEXCOORD2;
    
    [[vk::location(VS_OUT_SLOT_TBN_MATRIX)]]
    float3x3 tbn : TEXCOORD4;
};

VSOutput main(VSInput input)
{
    float3 T = normalize(mul((float3x3)instanceData.normalMatrix, input.tangent));
    float3 N = normalize(mul((float3x3)instanceData.normalMatrix, input.normal));
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(N, T));

    VSOutput output;
	output.colour = input.colour;
    output.texCoord = input.uv;
    output.position = mul(instanceData.modelMatrix, float4(input.position, 1.0)).xyz;
	output.tbn = float3x3(T, B, N);
	output.fragPos = output.position;
    output.svPosition = mul(frameData.projMatrix, mul(frameData.viewMatrix, mul(instanceData.modelMatrix, float4(input.position, 1.0))));

    return output;
}

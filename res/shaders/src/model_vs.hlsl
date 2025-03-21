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
    
	[[vk::location(VS_ATT_SLOT_BITANGENT)]]
    float3 bitangent : BITANGENT;
};

struct VSOutput
{
    float4 svPosition : SV_Position;

    [[vk::location(0)]] float3 colour       : COLOR;
    [[vk::location(1)]] float3 position     : TEXCOORD0;
    [[vk::location(2)]] float2 texCoord     : TEXCOORD1;
    [[vk::location(3)]] float3 fragPos      : TEXCOORD2;
    [[vk::location(4)]] float3x3 tbn        : TEXCOORD3;
};

VSOutput main(VSInput input)
{
    float3 T = normalize(mul((float3x3)instanceData.normalMatrix, input.tangent));
    float3 N = normalize(mul((float3x3)instanceData.normalMatrix, input.normal));
    float3 B = normalize(mul((float3x3)instanceData.normalMatrix, input.bitangent));

    VSOutput output;
	output.colour = input.colour;
    output.texCoord = input.uv;
    output.position = mul(instanceData.modelMatrix, float4(input.position, 1.0)).xyz;
	output.tbn = float3x3(T, B, N);
	output.fragPos = output.position;
    output.svPosition = mul(frameData.projMatrix, mul(frameData.viewMatrix, mul(instanceData.modelMatrix, float4(input.position, 1.0))));

    return output;
}

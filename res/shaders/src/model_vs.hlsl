#include "bindless.hlsl"

#include "model_common.inc"

struct VSInput
{
	[[vk::location(VS_MODEL_ATT_SLOT_POSITION)]]
    float3 position : POSITION;
    
	[[vk::location(VS_MODEL_ATT_SLOT_UV)]]
    float2 uv : TEXCOORD0;
    
	[[vk::location(VS_MODEL_ATT_SLOT_COLOUR)]]
    float3 colour : COLOR;
    
    [[vk::location(VS_MODEL_ATT_SLOT_NORMAL)]]
    float3 normal : NORMAL;
    
	[[vk::location(VS_MODEL_ATT_SLOT_TANGENT)]]
    float3 tangent : TANGENT;
    
	[[vk::location(VS_MODEL_ATT_SLOT_BITANGENT)]]
    float3 bitangent : BITANGENT;
};

struct VSOutput
{
    float4 svPosition : SV_Position;

	[[vk::location(VS_MODEL_OUT_SLOT_COLOR)]]
    float3 colour : COLOR;
    
	[[vk::location(VS_MODEL_OUT_SLOT_POSITION)]]
    float3 position : TEXCOORD0;
    
	[[vk::location(VS_MODEL_OUT_SLOT_UV)]]
    float2 texCoord : TEXCOORD1;
    
	[[vk::location(VS_MODEL_OUT_SLOT_TBN)]]
    float3x3 tbn : TEXCOORD3;
};

VSOutput main(VSInput input)
{
	FrameConstants frameData = bufferTable[pc.frameDataBuffer_ID].Load<FrameConstants>(0);
    TransformData transform = bufferTable[pc.transformBuffer_ID].Load<TransformData>(pc.transform_ID);
    
    float4x4 modelMatrix = transform.modelMatrix;
    float3x3 normalMatrix = (float3x3)transform.normalMatrix;
    
    float3 T = normalize(mul(normalMatrix, input.tangent));
    float3 N = normalize(mul(normalMatrix, input.normal));
    float3 B = normalize(mul(normalMatrix, input.bitangent));

    VSOutput output;
	output.colour = input.colour;
    output.texCoord = input.uv;
	output.position = mul(modelMatrix, float4(input.position, 1.0)).xyz;
	output.tbn = float3x3(T, B, N);
	output.svPosition = mul(frameData.projMatrix, mul(frameData.viewMatrix, mul(modelMatrix, float4(input.position, 1.0))));

    return output;
}

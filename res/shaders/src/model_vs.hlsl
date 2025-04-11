#include "bindless.hlsl"

#include "model_common.hlsl"

struct VSOutput
{
    float4 svPosition : SV_Position;

	[[vk::location(VS_MODEL_OUT_SLOT_COLOR)]]
    float3 colour : COLOR;
    
	[[vk::location(VS_MODEL_OUT_SLOT_POSITION)]]
    float4 position : TEXCOORD0;
    
	[[vk::location(VS_MODEL_OUT_SLOT_UV)]]
    float2 texCoord : TEXCOORD1;
    
	[[vk::location(VS_MODEL_OUT_SLOT_TBN)]]
    float3x3 tbn : TEXCOORD3;
};

VSOutput main(VSInput input)
{
	FrameConstants frameData = frameConstantsTable[pc.frameDataBuffer_ID][0];
    TransformData transform = transformTable[pc.transformBuffer_ID][pc.transform_ID];

    float4x4 modelMatrix = transform.modelMatrix;
    float3x3 normalMatrix = (float3x3)transform.normalMatrix;
    
    float3 T = normalize(mul(normalMatrix, input.tangent));
    float3 N = normalize(mul(normalMatrix, input.normal));
    float3 B = normalize(mul(normalMatrix, input.bitangent));

    VSOutput output;
	output.colour = input.colour;
    output.texCoord = input.uv;
	output.position = mul(modelMatrix, float4(input.position, 1.0));
	output.tbn = transpose(float3x3(T, B, N));
	output.svPosition = mul(frameData.projMatrix, mul(frameData.viewMatrix, mul(modelMatrix, float4(input.position, 1.0))));

    return output;
}

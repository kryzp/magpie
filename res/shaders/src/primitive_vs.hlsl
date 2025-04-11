#include "common.hlsl"

struct PushConstants
{
	float4x4 proj;
	float4x4 view;
};

PUSH_CONSTANTS(PushConstants, pc);

struct VSInput
{
	[[vk::location(0)]]
	float3 position : POSITION;
};

struct VSOutput
{
	float4 svPosition : SV_Position;
	
	[[vk::location(0)]]
	float3 worldPos : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.svPosition = mul(pc.proj, mul(pc.view, float4(input.position, 1.0)));
	output.worldPos = input.position;

	return output;
}

#include "common.hlsl"

#include "model_common.hlsl"

struct PushConstants
{
	float4x4 transform;
};

PUSH_CONSTANTS(PushConstants, pc);

float4 main(VSInput input) : SV_Position
{
	return mul(pc.transform, float4(input.position, 1.0));
}

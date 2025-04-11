#include "common.hlsl"

#include "model_common.hlsl"

struct PushConstants
{
	float4x4 proj;
	float4x4 view;
};

PUSH_CONSTANTS(PushConstants, pc);

float4 main(VSInput input) : SV_Position
{
	return mul(pc.proj, mul(pc.view, float4(input.position, 1.0)));
}

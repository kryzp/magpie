#include "shared/vertices.slang"

struct PushConstants
{
	float4x4 transform;
};

[[vk::push_constant]]
PushConstants pc;

[shader("vertex")]
float4 vertexMain(ModelVertex vertex) : SV_Position
{
	return mul(pc.transform, float4(vertex.position, 1.0));
}

#include "shared/vertices.slang"

struct Arguments
{
    float4x4 viewProj;
    float4 param;
};

[[vk::push_constant]]
Arguments args;

[[vk::binding(0)]]
SamplerCube skybox;

struct VS_Output
{
	float4 sv_position : SV_Position;

    [[vk::location(0)]]
    float3 position;
};

[shader("vertex")]
VS_Output vertexMain(PrimitiveVertex vertex)
{
	VS_Output output;

    output.position = vertex.position;

    output.sv_position = mul(args.viewProj, float4(vertex.position, 1.0));
    output.sv_position = output.sv_position.xyww;

	return output;
}

[shader("fragment")]
float4 fragmentMain(VS_Output input)
{
//    return float4(skybox.Sample(dir).rgb, 1.0);
    return float4(skybox.Sample(normalize(input.position * float3(1.0, 1.0, -1.0))).rgb, 1.0);
}

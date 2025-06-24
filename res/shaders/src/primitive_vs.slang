#include "shared/vertices.slang"

struct Arguments
{
    float4x4 viewProj;
};

[[vk::push_constant]]
Arguments args;

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

    return output;
}

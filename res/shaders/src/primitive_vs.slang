
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
    float3 worldPosition;
};

[shader("vertex")]
VS_Output vertexMain(float3 position)
{
    VS_Output output;

    output.sv_position = mul(args.viewProj, float4(position, 1.0));
    output.worldPosition = position;

    return output;
}


struct MyVertex
{
    [[vk::location(0)]]
	float3 position;
};

struct Arguments
{
	float4x4 transform_matrix;
};

[[vk::push_constant]]
Arguments args;

struct VS_Output
{
	float4 sv_position : SV_Position;
};

[shader("vertex")]
VS_Output VertexMain(MyVertex vertex)
{
	VS_Output output;
    output.sv_position = mul(args.transform_matrix, float4(vertex.position, 1.));

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
    return float4(1., 1., 1., 1.);
}

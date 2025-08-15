
struct MyVertex
{
    [[vk::location(0)]]
	float3 position;

	[[vk::location(1)]]
	float2 texcoord;
};

struct Arguments
{
	float4x4 transform_matrix;
	uint32_t texture_id;
};

[[vk::push_constant]]
Arguments args;

Sampler2D bindless_textures[];

struct VS_Output
{
	float4 sv_position : SV_Position;

	[[vk::location(0)]]
	float2 texcoord;
};

[shader("vertex")]
VS_Output VertexMain(MyVertex vertex)
{
	VS_Output output;
    output.sv_position = mul(args.transform_matrix, float4(vertex.position, 1.));
	output.texcoord = vertex.texcoord;

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
	return float4(bindless_textures[args.texture_id].Sample(input.texcoord).rgb, 1.);
}

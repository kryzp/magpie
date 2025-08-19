struct ModelVertex
{
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float2 texcoord;
    [[vk::location(2)]] float3 colour;
    [[vk::location(3)]] float3 normal;
    [[vk::location(4)]] float3 tangent;
    [[vk::location(5)]] float3 bitangent;
};

struct Arguments
{
	float4x4 transform;
};

[[vk::push_constant]]
Arguments args;

[[vk::binding(0)]] Texture2D bindless_images[];
[[vk::binding(1)]] SamplerState bindless_samplers[];

struct VS_Output
{
	float4 sv_position : SV_Position;
	[[vk::location(0)]] float3 normal;
};

[shader("vertex")]
VS_Output VertexMain(ModelVertex vertex)
{
	VS_Output output;
    output.sv_position = mul(args.transform, float4(vertex.position, 1.));
	output.normal = vertex.normal;

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	float3 colour = dot(input.normal, -float3(1., 0., 0.));
	return float4(colour, 1.);
}

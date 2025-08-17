struct EnvironmentMapVertex
{
    [[vk::location(0)]] float3 position;
};

struct Arguments
{
	float4x4 *transform_matrices;
};

[[vk::push_constant]]
Arguments args;

[[vk::binding(0)]]
Sampler2D bindless_textures[];

struct VS_Output
{
	float4 sv_position : SV_Position;
	[[vk::location(0)]] float3 position;
};

[shader("vertex")]
VS_Output VertexMain(EnvironmentMapVertex vertex, int view_id : SV_ViewID)
{
	VS_Output output;
    output.position = vertex.position;
    output.sv_position = mul(args.transform_matrices[view_id], float4(vertex.position, 1.0));

	return output;
}

float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(v.z, v.x), asin(v.y));
	uv *= float2(.1591, .3183);
	uv += .5;
	uv.y = 1. - uv.y;
	return uv;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	float2 uv = SampleSphericalMap(normalize(input.position));
	float3 colour = bindless_textures[0].Sample(uv).rgb;
	return float4(colour, 1.);
}

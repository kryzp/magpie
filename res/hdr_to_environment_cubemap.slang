#include "bindless.slang"

struct Arguments
{
	float4x4 *transform_matrices;
	float3 *vertices;
	uint hdr_image_id;
	uint linear_sampler_id;
};

[vk::push_constant]
Arguments args;

struct VS_Output
{
	float4 sv_position : SV_Position;
	[vk::location(0)] float3 position;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint view_id : SV_ViewID)
{
	VS_Output output;
	output.position = args.vertices[vertex_id];
	output.sv_position = mul(args.transform_matrices[view_id], float4(output.position, 1.));

	return output;
}

float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(-v.y, v.x), asin(v.z));
	uv *= float2(.1591, .3183);
	uv += .5;
	uv.y = 1. - uv.y;
	return uv;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	float2 uv = SampleSphericalMap(normalize(input.position));
	float3 colour = bindless_images[args.hdr_image_id].Sample(bindless_samplers[args.linear_sampler_id], uv).rgb;
	return float4(colour, 1.);
}

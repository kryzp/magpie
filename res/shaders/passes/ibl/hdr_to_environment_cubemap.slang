import bindless;
import ibl_convolution;

struct PushConstants
{
	float4x4 *transform_matrices;
	float3 *vertices;
	uint hdr_image_id;
	uint linear_sampler_id;
};

[vk::push_constant] PushConstants args;

[shader("vertex")]
ibl_convolution::VS_Output VertexMain(uint vertex_id : SV_VertexID, uint view_id : SV_ViewID)
{
	return ibl_convolution::CubemapFaceVS(vertex_id, view_id, args.transform_matrices, args.vertices);
}

float2 SampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(-v.y, v.x), asin(v.z));
	uv *= float2(.1591, .3183);
	uv += .5;
	uv.y = 1. - uv.y;
	return uv;
}

float4 DemoMap(float3 position)
{
//	return float4(position, 1.0);

	/*
	if (position.x >= 1.0)
		return float4(position.z, 0.0, 0.0, 1.0);

	if (position.y >= 1.0)
		return float4(0.0, position.z, 0.0, 1.0);

	if (position.z >= 1.0)
		return float4(0.0, 0.0, (max(position.y, 0.0) + max(position.x, 0.0)) * 0.5, 1.0);
	*/

	if (position.x >= 1.0) return float4(1.0, 0.0, 0.0, 1.0);
	if (position.y >= 1.0) return float4(0.0, 1.0, 0.0, 1.0);
	if (position.z >= 1.0) return float4(0.0, 0.0, 1.0, 1.0);

	return float4(0.0, 0.0, 0.0, 1.0);
}

[shader("fragment")]
float4 FragmentMain(ibl_convolution::VS_Output input) : SV_Target
{
//	return DemoMap(input.position);

	float2 uv = SampleSphericalMap(normalize(input.position));
	float3 colour = bdl::textures[args.hdr_image_id].Sample(bdl::samplers[args.linear_sampler_id], uv).rgb;

	return float4(colour, 1.0);
}

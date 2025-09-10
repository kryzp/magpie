#include "util.slang"
#include "bindless.slang"

struct Arguments {
	FrameData *frame_data;
	uint cubemap;
	uint sampler;
};

[vk::push_constant]
Arguments args;

struct VS_Output {
	float4 sv_position : SV_Position;
	[vk::location(0)] float3 position;
};

[shader("vertex")]
VS_Output VertexMain(float3 position)
{
	VS_Output output;

	output.position = position;
	
	output.sv_position = mul(args.frame_data->view_projection_no_translation, float4(position, 1.));
	output.sv_position = output.sv_position.xyww;
	
	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
	return float4(bindless_cubes[args.cubemap].Sample(bindless_samplers[args.sampler], normalize(WorldToCubemap(input.position))).rgb, 1.);
}

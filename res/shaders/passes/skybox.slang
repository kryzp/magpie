import util;
import bindless;
import types;

struct Arguments {
    types::FrameData *frame_data;
    float3 *vertices;
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
VS_Output VertexMain(uint vertex_id : SV_VertexID)
{
	VS_Output output;
	output.position = args.vertices[vertex_id];

	output.sv_position = mul(args.frame_data->view_projection_no_translation, float4(output.position, 1.));
	output.sv_position = output.sv_position.xyww;

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
    return float4(bindless::cubes[args.cubemap].Sample(bindless::samplers[args.sampler], normalize(util::WorldToCubemap(input.position))).rgb, 1.);
}

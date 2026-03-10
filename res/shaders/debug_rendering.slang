#include "util.slang"
#include "bindless.slang"

struct Arguments {
    float4x4 view_proj;
    float4 from_position;
	float4 to_position;
    float4 colour;
    float2 resolution;
	float thickness;
};

[vk::push_constant]
Arguments args;

struct VS_Output {
	float4 sv_position : SV_Position;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID)
{
    float4 clip0 = mul(args.view_proj, args.from_position);
    float4 clip1 = mul(args.view_proj, args.to_position);

    float2 p0 = clip0.xy / clip0.w * args.resolution;
    float2 p1 = clip1.xy / clip1.w * args.resolution;

    float2 dir = normalize(p1 - p0);
    float2 norm = float2(-dir.y, dir.x);

    float2 offset = norm * args.thickness / args.resolution;

    bool start = vertex_id == 0 || vertex_id == 1;
    bool left  = vertex_id == 0 || vertex_id == 2;

    float4 pos = start ? clip0 : clip1;
    float2 off = left ? offset : -offset;

    pos.xy += off * pos.w;

    VS_Output output;
    output.sv_position = pos;

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
	return args.colour;
}

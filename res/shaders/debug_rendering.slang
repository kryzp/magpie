#include "util.slang"
#include "bindless.slang"

struct DebugLineDraw {
    float4 from;
    float4 to;
    float4 colour;
    float thickness;
};

struct Arguments {
    float4x4 view_proj;
    DebugLineDraw *calls;
    float2 resolution;
};

[vk::push_constant]
Arguments args;

struct VS_Output {
    [vk::location(0)] float4 colour; 
	float4 sv_position : SV_Position;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint instance : SV_VulkanInstanceID)
{
    DebugLineDraw call = args.calls[instance];

    float4 clip0 = mul(args.view_proj, call.from);
    float4 clip1 = mul(args.view_proj, call.to);

    float2 p0 = clip0.xy / clip0.w * args.resolution;
    float2 p1 = clip1.xy / clip1.w * args.resolution;

    float2 dir = normalize(p1 - p0);
    float2 norm = float2(-dir.y, dir.x);

    float2 offset = norm * call.thickness / args.resolution;

    bool start = vertex_id == 0 || vertex_id == 1;
    bool left  = vertex_id == 0 || vertex_id == 2;

    float4 pos = start ? clip0 : clip1;
    float2 off = left ? offset : -offset;

    pos.xy += off * pos.w;

    VS_Output output;
    output.colour = call.colour;
    output.sv_position = pos;

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
	return input.colour;
}

import bindless;
import types;

struct DebugObjectDraw {
    float4x4 transform;
    float4 colour;
    float thickness;
};

struct Arguments {
    float4x4 view_proj;
    types::DebugObjectDraw *calls;
    float3 *vertices;
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
    types::DebugObjectDraw call = args.calls[instance];

    VS_Output output;
    output.colour = call.colour;
    output.sv_position = mul(args.view_proj, mul(call.transform, float4(args.vertices[vertex_id], 1.0)));

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
	return input.colour;
}

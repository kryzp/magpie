import bindless;

struct CanvasVertex {
	float2 position;
	float2 uv;
	float4 colour;
	uint texture;
	uint pad0;
	uint pad1;
	uint pad2;
};

struct CanvasArgs {
	float4x4 projection;
	CanvasVertex *vertices;
	uint sampler;
};

[[vk::push_constant]]
CanvasArgs args;

struct VertexOut {
	float4 position : SV_Position;
	float2 uv;
	float4 colour;
	nointerpolation uint texture;
};

[shader("vertex")]
VertexOut VertexMain(uint vertex_id : SV_VertexID)
{
	CanvasVertex v = args.vertices[vertex_id];

	VertexOut out;
	out.position = mul(args.projection, float4(v.position, 0.0, 1.0));
	out.uv = v.uv;
	out.colour = v.colour;
	out.texture = v.texture;
	return out;
}

[shader("fragment")]
float4 FragmentMain(VertexOut in) : SV_Target0
{
	return SampleBindless(in.texture, args.sampler, in.uv) * in.colour;
}

import bindless;

struct Arguments {
	uint width;
	uint height;
	float exposure;
	uint input_image_id;
	uint output_image_id;
};

[vk::push_constant]
Arguments args;

float3 Tonemapping(float3 c)
{
	float3 col = c;

    col = 1.0 - exp(-col * args.exposure);
//	col = 1.0 - col;

    return col;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeMain(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x >= args.width || tid.y >= args.height)
		return;

	Texture2D input = bindless::images[args.input_image_id];
	RWTexture2D output = bindless::rw_images[args.output_image_id];
	
	output[tid.xy] = float4(Tonemapping(input[tid.xy].rgb).rgb, 1.0);
}

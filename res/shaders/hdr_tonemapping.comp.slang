#include "bindless.slang"

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
	float3 out = c;
	
	out = 1.0 - exp(-out * args.exposure);
//	out = 1.0 - out;

	return out;
}

[shader("compute")]
[numthreads(1, 1, 1)]
void ComputeMain(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x < 0 || tid.x >= args.width ||
	    tid.y < 0 || tid.y >= args.height)
		return;

	Texture2D input = bindless_images[args.input_image_id];
	RWTexture2D output = bindless_rw_images[args.output_image_id];
	
	output[tid.xy] = float4(Tonemapping(input[tid.xy].rgb).rgb, 1.0);
}

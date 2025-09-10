#include "bindless.slang"

struct Arguments {
	uint width;
	uint height;
	float exposure;
	uint image_id;
};

[vk::push_constant]
Arguments args;

float3 Tonemapping(float3 c)
{
	return 1. - exp(-c * args.exposure);
}

[shader("compute")]
[numthreads(1, 1, 1)]
void ComputeMain(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x < 0 || tid.x >= args.width ||
	    tid.y < 0 || tid.y >= args.height)
		return;

	RWTexture2D image = bindless_rw_images[args.image_id];

	image[tid.xy] = float4(Tonemapping(image[tid.xy].rgb).rgb, 1.);
}


struct Arguments
{
    uint width;
    uint height;
    float exposure;
};

[vk::push_constant]
Arguments args;

[[vk::binding(0)]]
RWTexture2D<float4> target;

float3 tonemapping(float3 c)
{
    return 1.0 - exp(-c * args.exposure);
}

[shader("compute")]
[numthreads(1, 1, 1)]
void computeMain(uint3 tid : SV_DispatchThreadID)
{
    if (tid.x < 0 || tid.x >= args.width ||
		tid.y < 0 || tid.y >= args.height)
	{
		return;
	}

    target[tid.xy] = float4(tonemapping(target[tid.xy].rgb), 1.0);
}

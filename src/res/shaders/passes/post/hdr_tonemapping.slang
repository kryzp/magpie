import bindless;

struct PushConstants {
	uint width;
	uint height;
	float exposure;
	uint input_image_id;
	uint output_image_id;
	uint encode_srgb;
};

[vk::push_constant]
PushConstants args;

float3 ApplyTonemapping(float3 c)
{
//	return 1.0 - exp(-c);
	return saturate((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14)); // ACES fitted
}

float3 LinearToSRGB(float3 c)
{
    c = saturate(c);
    float3 lo = c * 12.92;
    float3 hi = 1.055 * pow(c, 1.0 / 2.4) - 0.055;
    return lerp(hi, lo, step(c, 0.0031308));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void ComputeMain(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x >= args.width || tid.y >= args.height)
		return;

    float3 c = TextureLoadBindless(args.input_image_id, tid.xy).rgb;

    c = ApplyTonemapping(c * args.exposure);

    if (args.encode_srgb != 0)
        c = LinearToSRGB(c);

    StorageWriteBindless(args.output_image_id, tid.xy, float4(c, 1.0));
}

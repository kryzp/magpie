module bindless;
import math;

[[vk::binding(0, 0)]] public SamplerState bindless_samplers[];
[[vk::binding(0, 0)]] public SamplerComparisonState bindless_cmp_samplers[];
[[vk::binding(1, 0)]] public Texture2D bindless_textures[];
[[vk::binding(2, 0)]] public TextureCube bindless_cubemaps[];
[[vk::binding(3, 0)]] public RWTexture2D<float4> bindless_storage[];

public bool BindlessIsValid(uint id)
{
	return id != 0;
}

public float4 SampleBindless(uint texture, uint sampler, float2 uv)
{
	return bindless_textures[NonUniformResourceIndex(texture)].Sample(bindless_samplers[NonUniformResourceIndex(sampler)], uv);
}

public float4 SampleBindlessLevel(uint texture, uint sampler, float2 uv, float level)
{
	return bindless_textures[NonUniformResourceIndex(texture)].SampleLevel(bindless_samplers[NonUniformResourceIndex(sampler)], uv, level);
}

// NOTE: SAMPLING BINDLESS CUBE AUTOMATICALLY APPLIES WORLDTOCUBEMAP!

public float4 SampleBindlessCube(uint cube, uint sampler, float3 direction)
{
	return bindless_cubemaps[NonUniformResourceIndex(cube)].Sample(bindless_samplers[NonUniformResourceIndex(sampler)], math::WorldToCubemap(direction));
}

public float4 SampleBindlessCubeLevel(uint cube, uint sampler, float3 direction, float level)
{
	return bindless_cubemaps[NonUniformResourceIndex(cube)].SampleLevel(bindless_samplers[NonUniformResourceIndex(sampler)], math::WorldToCubemap(direction), level);
}

public float SampleBindlessCubeCmpUniform(uint cube, uint cmp_sampler, float3 direction, float reference)
{
	return bindless_cubemaps[NonUniformResourceIndex(cube)].SampleCmpLevelZero(bindless_cmp_samplers[NonUniformResourceIndex(cmp_sampler)], math::WorldToCubemap(direction), reference);
}

public float4 TextureLoadBindless(uint texture, int2 texel)
{
	return bindless_textures[NonUniformResourceIndex(texture)].Load(int3(texel, 0));
}

public float4 TextureLoadBindlessLevel(uint texture, int2 texel, int mip)
{
	return bindless_textures[NonUniformResourceIndex(texture)].Load(int3(texel, mip));
}

public void StorageWriteBindless(uint storage, int2 texel, float4 value)
{
	bindless_storage[NonUniformResourceIndex(storage)][texel] = value;
}

public uint2 BindlessTextureSize(uint texture)
{
	uint w, h;
	bindless_textures[NonUniformResourceIndex(texture)].GetDimensions(w, h);
	return uint2(w, h);
}

public uint2 BindlessCubeSize(uint cube)
{
	uint w, h;
	bindless_cubemaps[NonUniformResourceIndex(cube)].GetDimensions(w, h);
	return uint2(w, h);
}

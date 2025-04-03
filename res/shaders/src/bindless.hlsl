#include "common.hlsl"

struct MaterialData
{
	int diffuseTexture_ID;
	int aoTexture_ID;
	int armTexture_ID;
	int normalTexture_ID;
	int emissiveTexture_ID;
};

struct PushConstants
{
	int frameDataBuffer_ID;
	int transformBuffer_ID;
	int materialDataBuffer_ID;
	
	int transform_ID;
	
	int irradianceMap_ID;
	int prefilterMap_ID;
	
	int brdfLUT_ID;
	
	int material_ID;
	
	int cubemapSampler_ID;
	int textureSampler_ID;
};

PUSH_CONSTANTS(PushConstants, pc);

ByteAddressBuffer	bufferTable[]		: register(t0);
SamplerState		samplerTable[]		: register(t1);
Texture2D			texture2DTable[]	: register(t2);
TextureCube			textureCubeTable[]	: register(t3);

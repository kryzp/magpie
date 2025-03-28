#include "common_fxc.inc"

#define MATH_PI 3.14159265359
#define MAX_N_LIGHTS 16

struct Light
{
    float3 position;
    float radius;
    float3 colour;
    float attenuation;
    float3 direction;
    float type;
};

struct FrameUBO
{
    float4x4 projMatrix;
    float4x4 viewMatrix;
    float4 viewPos;
//    Light lights[MAX_N_LIGHTS];
};

struct TransformData
{
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

// constant data
ConstantBuffer<FrameUBO> frameData : register(b0);

// "bindless" indexed data
StructuredBuffer<TransformData> transformTable : register(t1);
Texture2D texture2DTable[] : register(t2);
TextureCube textureCubeTable[] : register(t3);
SamplerState samplerTable[] : register(t4);

struct PushConstants
{
	int transform_ID;
	
	int irradianceMap_ID;
	int prefilterMap_ID;
	
	int brdfLUT_ID;
	
	int diffuseTexture_ID;
	int aoTexture_ID;
	int mrTexture_ID;
	int normalTexture_ID;
	int emissiveTexture_ID;
	
	int cubemapSampler_ID;
	int textureSampler_ID;
};

[[vk::push_constant]]
PushConstants pc;

#include "common.hlsl"

struct PushConstants
{
	int frameDataBuffer_ID;
	int transformBuffer_ID;
	int materialDataBuffer_ID;
	int lightBuffer_ID;
	
	int transform_ID;
	
	int irradianceMap_ID;
	int prefilterMap_ID;
	
	int brdfLUT_ID;
	
	int material_ID;
	
	int cubemapSampler_ID;
	int textureSampler_ID;
	int shadowAtlasSampler_ID;

	int shadowAtlas_ID;
};

PUSH_CONSTANTS(PushConstants, pc);

StructuredBuffer<FrameConstants> frameConstantsTable[] : register(t0);
StructuredBuffer<TransformData> transformTable[] : register(t0);
StructuredBuffer<MaterialData> materialDataTable[] : register(t0);
StructuredBuffer<PointLight> lightsTable[] : register(t0);

SamplerState samplerTable[] : register(t1);
Texture2D texture2DTable[] : register(t2);
TextureCube textureCubeTable[] : register(t3);

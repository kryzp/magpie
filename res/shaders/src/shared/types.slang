#ifndef TYPES_SLANG_
#define TYPES_SLANG_

struct FrameData
{
	float4x4 proj;
	float4x4 view;
	float4 viewPosition;
};

struct TransformData
{
	float4x4 modelMatrix;
	float4x4 normalMatrix;
};

struct MaterialData
{
	uint diffuse_id;
    uint ambient_id;
    uint material_id;
	uint normal_id;
	uint emissive_id;
};

struct PointLight
{
	float4 position;
	float4 colour;
	float4 attenuation;
//	float4x4 lightSpaceMatrix;
//	int4 shadowMap_id;
};

struct BufferPointers
{
    FrameData *frameData;
    TransformData *transforms;
    MaterialData *materials;
    PointLight *pointLights;
};

#endif // TYPES_SLANG_

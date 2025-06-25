#ifndef TYPES_SLANG_
#define TYPES_SLANG_

struct FrameData
{
	float4x4 proj;
	float4x4 view;
	float4 cameraPosition;
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
};

#endif // TYPES_SLANG_

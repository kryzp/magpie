#ifndef COMMON_HLSL_
#define COMMON_HLSL_

#define PUSH_CONSTANTS(__pc_type, __pc_name) [[vk::push_constant]] __pc_type __pc_name;

#define MATH_PI 3.14159265359

#define MAX_POINT_LIGHTS 16

struct PointLight
{
    float4 position;
    float4 colour;
};

struct MaterialData
{
	int diffuseTexture_ID;
	int aoTexture_ID;
	int armTexture_ID;
	int normalTexture_ID;
	int emissiveTexture_ID;
};

struct FrameConstants
{
    float4x4 projMatrix;
    float4x4 viewMatrix;
    float4 viewPos;
};

struct TransformData
{
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

#endif // COMMON_HLSL_

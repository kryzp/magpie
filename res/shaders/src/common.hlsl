#ifndef COMMON_HLSL_
#define COMMON_HLSL_

#define PUSH_CONSTANTS(__pc_type, __pc_name) [[vk::push_constant]] __pc_type __pc_name;

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

struct FrameConstants
{
    float4x4 projMatrix;
    float4x4 viewMatrix;
    float4 viewPos;
//	Light lights[MAX_N_LIGHTS];
};

struct TransformData
{
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

#endif // COMMON_HLSL_

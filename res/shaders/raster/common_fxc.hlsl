#include "common_fxc.inc"

struct PushConstants
{
    int storage;
    int texture2D_RID;
    int cubeMap_RID;
};

[[vk::push_constant]]
PushConstants puchConstants;

#define MAX_N_LIGHTS 16

struct Light
{
    float3 position;
    float radius;
    float3 colour;
    float attenuation;
    float3 direction;
    float type; // 0 - sun, 1 - spotlight, 2 - point
};

struct FrameUBO
{
    float4x4 projMatrix;
    float4x4 viewMatrix;
    float4 viewPos;
    Light lights[MAX_N_LIGHTS];
};

cbuffer frameData : register(b0) { FrameUBO frameData; };

struct InstanceUBO
{
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

cbuffer instanceData : register(b1) { InstanceUBO instanceData; };

#include "shared/types.slang"
#include "shared/bindless.slang"
#include "shared/vertices.slang"

struct ModelBuffers
{
    FrameData *frameData;
    TransformData *transforms;
    MaterialData *materials;
}

struct ModelPushConstants
{
    ModelBuffers *buffers;

    uint irradianceMap_id;
    uint prefilterMap_id;
    uint brdfLUT_id;
    uint material_id;
    uint textureSampler_id;
    uint cubemapSampler_id;
};

[[vk::push_constant]]
ModelPushConstants g_bindless;

struct VS_Output
{
    float4 sv_position : SV_Position;

    [[vk::location(0)]] float4 position;
    [[vk::location(1)]] float2 uv;
    [[vk::location(2)]] float3 colour;
    [[vk::location(3)]] float3x3 tbn;
};

[shader("vertex")]
VS_Output vertexMain(ModelVertex vertex, uint instanceID : SV_InstanceID)
{
    FrameData *frameData = g_bindless.buffers.frameData;
    TransformData *transform = g_bindless.buffers.transforms + instanceID;

    float4x4 projMatrix = frameData.proj;
    float4x4 viewMatrix = frameData.view;
    float4x4 modelMatrix = transform.modelMatrix;
    float3x3 normalMatrix = (float3x3)transform.normalMatrix;

    float3 T = normalize(mul(normalMatrix, vertex.tangent));
    float3 B = normalize(mul(normalMatrix, vertex.bitangent));
    float3 N = normalize(mul(normalMatrix, vertex.normal));

    VS_Output output;
    output.position = mul(modelMatrix, float4(vertex.position, 1.0));
    output.sv_position = mul(projMatrix, mul(viewMatrix, output.position));
    output.uv = vertex.uv;
    output.colour = vertex.colour;
    output.tbn = transpose(float3x3(T, B, N));

    return output;
}

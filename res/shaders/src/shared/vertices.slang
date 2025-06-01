#ifndef VERTICES_SLANG_
#define VERTICES_SLANG_

struct PrimitiveVertex
{
    [[vk::location(0)]] float3 position;
};

struct PrimitiveUVVertex
{
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float2 uv;
};

struct ModelVertex
{
    [[vk::location(0)]] float3 position;
    [[vk::location(1)]] float2 uv;
    [[vk::location(2)]] float3 colour;
    [[vk::location(3)]] float3 normal;
    [[vk::location(4)]] float3 tangent;
    [[vk::location(5)]] float3 bitangent;
};

#endif // VERTICES_SLANG_

#include "primitive_vs.slang"

[[vk::binding(0)]]
Sampler2D texture;

float2 sampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= float2(0.1591, 0.3183);
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

[shader("fragment")]
float4 fragmentMain(VS_Output input) : SV_Target
{
    float2 uv = sampleSphericalMap(normalize(input.worldPosition));
    float3 col = texture.Sample(uv).rgb;
    return float4(col, 1.0);
}

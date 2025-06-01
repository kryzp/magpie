#include "fullscreen_triangle_vs.slang"

[[vk::binding(0)]]
Sampler2D texture;

[shader("fragment")]
float4 fragmentMain(VS_Output input)
{
    return texture.Sample(input.uv);
}

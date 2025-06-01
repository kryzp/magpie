#include "model_shadow_map_vs.slang"

[shader("fragment")]
float fragmentMain(float4 sv_position : SV_Position) : SV_Depth
{
    return sv_position.z;
}

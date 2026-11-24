module shadows;

import bindless;
import math;

namespace shadows {

public static const uint32_t SHADOW_STREAM_OPAQUE = 0;
public static const uint32_t SHADOW_STREAM_MASKED = 1;
public static const uint32_t SHADOW_STREAM_COUNT  = 2;

public struct ShadowCaster {
	public float4x4 face_matrices[6]; // reversed-z
	public float3 position;
	public float near_plane;
	public float far_plane;
	public uint shadow_map;
	public float texel_world_scale; // world size of one texel at distance 1 from the light
	public float cull_radius;
};

public static const float SHADOW_PCF_RADIUS_TEXELS = 1.5;
public static const float SHADOW_NORMAL_OFFSET_TEXELS = 1.0;

public static const int SHADOW_PCF_TAPS = 8;

static const float2 PCF_DISK[SHADOW_PCF_TAPS] = {
	float2( 0.2500,  0.0000),
	float2(-0.3193,  0.2925),
	float2( 0.0489, -0.5569),
	float2( 0.4024,  0.5249),
	float2(-0.7385, -0.1306),
	float2( 0.6996, -0.4450),
	float2(-0.2340,  0.8705),
	float2(-0.4463, -0.8593)
};

public float CubeFaceDepthHere(float3 light_to_frag, float near_plane, float far_plane)
{
	float3 a = abs(light_to_frag);
	float z = max(a.x, max(a.y, a.z));
    // (we use reversed-z)
	return (near_plane / (far_plane - near_plane)) * (far_plane / z - 1.0);
}

// returns shadow multiplier (out = shadow * in)
public float CalculatePointShadow(
	float3 frag_position,
	float3 geometric_normal,
	ShadowCaster caster,
	uint cmp_sampler
)
{
	float3 light_to_frag = frag_position - caster.position;

	float3 a = abs(light_to_frag);
	float major_distance = max(a.x, max(a.y, a.z));

	float texel = caster.texel_world_scale * major_distance;

	float3 dir = light_to_frag + geometric_normal * (texel * SHADOW_NORMAL_OFFSET_TEXELS);

	float reference = CubeFaceDepthHere(dir, caster.near_plane, caster.far_plane);

	float3 t, b;
	math::BuildOrthonormalBasis(normalize(dir), t, b);

	float radius = texel * SHADOW_PCF_RADIUS_TEXELS;

	float lit = 0.0;

	[unroll]
	for (int i = 0; i < SHADOW_PCF_TAPS; i++) {
		float3 sample_dir = dir + (t*PCF_DISK[i].x + b*PCF_DISK[i].y) * radius;
		lit += SampleBindlessCubeCmpUniform(caster.shadow_map, cmp_sampler, sample_dir, reference);
	}

	return lit * (1.0 / float(SHADOW_PCF_TAPS));
}

}

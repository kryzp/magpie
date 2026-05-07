module shadows;

import frame;
import bindless;
import math;

namespace shadows
{
	public struct ShadowCaster
	{
		public float4x4 face_matrices[6];
		public float3 position;
		public float near_plane;
		public float far_plane;
		public int shadow_map;
	};

	public static const float3 PCF_OFFSETS[20] = {
		float3( 1.0, 1.0, 1.0), float3( 1.0,-1.0, 1.0), float3(-1.0,-1.0,  1.0), float3(-1.0, 1.0, 1.0),
		float3( 1.0, 1.0,-1.0), float3( 1.0,-1.0,-1.0), float3(-1.0,-1.0, -1.0), float3(-1.0, 1.0,-1.0),
		float3( 1.0, 1.0, 0.0), float3( 1.0,-1.0, 0.0), float3(-1.0,-1.0,  0.0), float3(-1.0, 1.0, 0.0),
		float3( 1.0, 0.0, 1.0), float3(-1.0, 0.0, 1.0), float3( 1.0, 0.0, -1.0), float3(-1.0, 0.0,-1.0),
		float3( 0.0, 1.0, 1.0), float3( 0.0,-1.0, 1.0), float3( 0.0,-1.0, -1.0), float3( 0.0, 1.0,-1.0)
	};

	// PCF point-light shadow lookup.
	// returns shadow multiplier (out = shadow * in)
	public float CalculatePointShadow(float3 frag_position,
	                                  ShadowCaster caster,
	                                  frame::FrameData frame_data,
	                                  uint shadow_sampler_id)
	{
		SamplerState shadow_sampler = bdl::samplers[shadow_sampler_id];
		TextureCube shadow_map = bdl::cubes[caster.shadow_map];

		float3 light_to_frag = frag_position - caster.position;
		float current_depth = length(light_to_frag);

		float shadow = 0.0;

		float bias = 0.05;
		const int sample_count = 20;

		float disk_radius = (1.0 + (current_depth / caster.far_plane) * 100.0) * 0.0005;

		for (int i = 0; i < sample_count; i++)
		{
			float3 sample_dir = light_to_frag + PCF_OFFSETS[i] * disk_radius;
			sample_dir = normalize(sample_dir);

			float sampled_depth = shadow_map.Sample(shadow_sampler, math::WorldToCubemap(sample_dir)).r;
			sampled_depth = caster.near_plane + sampled_depth * (caster.far_plane - caster.near_plane);

			if (current_depth - bias > sampled_depth)
				shadow += 1.0;
		}

		shadow /= float(sample_count);

		return saturate(1.0 - shadow);
	}
}

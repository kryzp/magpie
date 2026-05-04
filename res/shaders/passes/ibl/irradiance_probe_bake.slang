/*
 * - SH basis functions: Ramamoorthi & Hanrahan (2001)
 *   "An Efficient Representation for Irradiance Environment Maps"
 *
 * - Fibonacci sphere for uniform ray distribution:
 *   https://extremelearning.com.au/how-to-evenly-distribute-points-on-a-sphere-more-effectively-than-the-canonical-fibonacci-lattice/
 *
 * - VK_KHR_ray_query usage:
 *   https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/extensions/ray_queries
 */

import bindless;
import math;
import irradiance;

struct PushConstants
{
	irradiance::ProbeSH *sh_buffer;
	irradiance::ProbeGridInfo *grid_info_buffer;
	uint64_t tlas_address;

	uint environment_cubemap;
	uint linear_sampler;

	uint rays_per_probe;
	uint _padding;
};

[[vk::push_constant]] PushConstants pc;

// we can use a fibonacic lattice distribution to get a
// nicer uniform distribution of points on the sphere.
float3 FibonacciSphereDirection(uint index, uint total)
{
	// Golden ratio based distribution.
	static const float GOLDEN_RATIO = 1.618033988749895;

	float theta = 2.0 * math::PI * (float(index) / GOLDEN_RATIO);
	float phi = acos(1.0 - 2.0 * (float(index) + 0.5) / float(total));

	float sin_phi = sin(phi);

	return float3(sin_phi * cos(theta),
	              sin_phi * sin(theta),
	              cos(phi));
}

float3 ProbeWorldPosition(uint probe_index, irradiance::ProbeGridInfo grid)
{
	uint iz = probe_index / (grid.count_x * grid.count_y);
	uint iy = (probe_index / grid.count_x) % grid.count_y;
	uint ix = probe_index % grid.count_x;

	return grid.grid_min + float3(float(ix) * grid.cell_size.x,
	                              float(iy) * grid.cell_size.y,
	                              float(iz) * grid.cell_size.z);
}

[shader("compute")]
[numthreads(64, 1, 1)]
void ComputeMain(uint3 dispatch_id : SV_DispatchThreadID)
{
	uint probe_index = dispatch_id.x;

	irradiance::ProbeGridInfo grid = *pc.grid_info_buffer;

	if (probe_index >= grid.total_probe_count)
		return;

	float3 probe_pos = ProbeWorldPosition(probe_index, grid);

	float3 sh_accum[9];
	for (uint i = 0; i < 9; i++)
		sh_accum[i] = float3(0, 0, 0);

	RaytracingAccelerationStructure scene_as = RaytracingAccelerationStructure(pc.tlas_address);

	SamplerState env_sampler = bdl::samplers[pc.linear_sampler];
	TextureCube env_map = bdl::cubes[pc.environment_cubemap];

	for (uint ray_idx = 0; ray_idx < pc.rays_per_probe; ray_idx++)
	{
		float3 ray_dir = FibonacciSphereDirection(ray_idx, pc.rays_per_probe);

		RayDesc ray;
		ray.Origin = probe_pos;
		ray.Direction = ray_dir;
		ray.TMin = 0.01; // avoid self-intersection
		ray.TMax = 1000.0;

		RayQuery<RAY_FLAG_FORCE_OPAQUE> rq;
		rq.TraceRayInline(scene_as, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);
		rq.Proceed();

		float3 radiance = float3(0, 0, 0);

		if (rq.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
		{
			/* TODO: basically im supposed to do the following here:
			         -> get instance index
			         -> get triangle index
			         -> get barycentrices
			         -> get vertex data + interpolate uvs
			         -> get the albedo at that ponit
			         -> apply diffuse model (lambert * light)
			*/
			radiance = 0.0;
			//radiance = env_map.SampleLevel(env_sampler, math::WorldToCubemap(ray_dir), 0).rgb;
		}
		else
		{
			radiance = env_map.SampleLevel(env_sampler, math::WorldToCubemap(ray_dir), 0).rgb;
		}

		// projection step (radiance -> basis)
		float basis[9];
		irradiance::SH_EvaluateBasis(ray_dir, basis);

		// monte carlo weight is 4*pi/N == (sphere angle) / (# of samples)
		float weight = (4.0 * math::PI) / float(pc.rays_per_probe);

		for (uint c = 0; c < 9; c++)
			sh_accum[c] += radiance * basis[c] * weight;
	}

	irradiance::ProbeSH *output = pc.sh_buffer;

	for (uint c = 0; c < 9; c++)
		output[probe_index].coefficients[c] = float4(sh_accum[c] * irradiance::SH_COS_CONV[c], 0.0);
}

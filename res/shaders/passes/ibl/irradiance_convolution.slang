import bindless;
import math;
import ibl_convolution;

struct PushConstants
{
	float4x4 *transform_matrices;
	float3 *vertices;
	uint environment_map_id;
	uint linear_sampler_id;
};

[vk::push_constant] PushConstants args;

[shader("vertex")]
ibl_convolution::VS_Output VertexMain(uint vertex_id : SV_VertexID, uint view_id : SV_ViewID)
{
	return ibl_convolution::CubemapFaceVS(vertex_id, view_id, args.transform_matrices, args.vertices);
}

[shader("fragment")]
float4 FragmentMain(ibl_convolution::VS_Output input)
{
	TextureCube env_map = bdl::cubes[args.environment_map_id];
	SamplerState sampler = bdl::samplers[args.linear_sampler_id];

	float3 irradiance = 0.0;

	float3 forward = normalize(input.position);

	float3 right, up;
	math::BuildOrthonormalBasis(forward, right, up);

	float dt = 0.025;
	float n_samples = 0.0;

	for (float phi = 0.0; phi < 2.0 * math::PI; phi += dt)
	{
		for (float theta = 0.0; theta < 0.5 * math::PI; theta += dt)
		{
			float3 sample = float3(
				sin(theta) * cos(phi),
				sin(theta) * sin(phi),
				cos(theta)
			);

			float3x3 tangent_to_world = transpose(float3x3(right, up, forward));
			float3 sample_direction = mul(tangent_to_world, sample);
			sample_direction = math::WorldToCubemap(sample_direction);

			irradiance += env_map.Sample(sampler, sample_direction).rgb * cos(theta) * sin(theta);

			n_samples++;
		}
	}

	irradiance *= math::PI / n_samples;

	return float4(irradiance, 1.0);
}

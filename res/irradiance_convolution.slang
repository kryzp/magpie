#include "util.slang"

struct Arguments
{
	float4x4 *transform_matrices;
	uint environment_map_id;
	uint linear_sampler_id;
	uint _padding[2];
};

[[vk::push_constant]]
Arguments args;

[[vk::binding(0)]] SamplerState bindless_samplers[];
[[vk::binding(2)]] TextureCube bindless_cubes[];

struct VS_Output
{
	float4 sv_position : SV_Position;

	[[vk::location(0)]] float3 position;
};

[shader("vertex")]
VS_Output VertexMain(float3 position, int view_id : SV_ViewID)
{
	VS_Output output;
    output.position = position;
    output.sv_position = mul(args.transform_matrices[view_id], float4(position, 1.));

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
	float3 forward = normalize(input.position);
	float3 right = normalize(cross(forward, float3(0., 0., 1.)));
	float3 up = normalize(cross(right, forward));

	float dt = .1;
	float n_samples = 0.;

	float3 irradiance = 0.;
	
	for(float phi = 0.; phi < 2. * PI; phi += dt)
	{
		for(float theta = 0.; theta < .5 * PI; theta += dt)
		{
			float3 sample = float3(
				sin(theta) * cos(phi),
				sin(theta) * sin(phi),
				cos(theta)
			);

			float3x3 tangent_to_world = transpose(float3x3(right, forward, up));

			float3 sample_direction = mul(tangent_to_world, sample);

			irradiance += bindless_cubes[args.environment_map_id].Sample(bindless_samplers[args.linear_sampler_id], WorldToCubemap(sample_direction)).rgb * cos(theta) * sin(theta);

			n_samples++;
		}
	}

	irradiance *= PI / n_samples;

	return float4(irradiance, 1.);
}

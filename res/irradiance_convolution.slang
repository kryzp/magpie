#include "util.slang"

struct EnvironmentMapVertex
{
    [[vk::location(0)]] float3 position;
};

struct Arguments
{
	float4x4 *transform_matrices;
	uint environment_map_id;
	uint linear_sampler_id;
	uint _padding[2];
};

[[vk::push_constant]]
Arguments args;

[[vk::binding(0)]] TextureCube bindless_images[];
[[vk::binding(1)]] SamplerState bindless_samplers[];

struct VS_Output
{
	float4 sv_position : SV_Position;
	[[vk::location(0)]] float3 position;
};

[shader("vertex")]
VS_Output VertexMain(EnvironmentMapVertex vertex, int view_id : SV_ViewID)
{
	VS_Output output;
    output.position = vertex.position;
    output.sv_position = mul(args.transform_matrices[view_id], float4(vertex.position, 1.));

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
	float3 normal = normalize(input.position);
	float3 right = normalize(cross(float3(0.0, 1.0, 0.0), normal));
	float3 up = normalize(cross(normal, right));

	float dt = .1;
	float n_samples = 0.;

	float3 irradiance = 0.;
	
	for (float phi = 0.; phi < 2. * PI; phi += dt)
	{
		for (float theta = 0.0; theta < .5 * PI; theta += dt)
		{
			float3 sample = float3(
				sin(theta) * cos(phi),
				sin(theta) * sin(phi),
				cos(theta)
			);
			
			float3x3 tangent_to_world = transpose(float3x3(right, up, normal));
			
			float3 sample_direction = mul(tangent_to_world, sample);

			irradiance += bindless_images[args.environment_map_id].Sample(bindless_samplers[args.linear_sampler_id], WorldToCubemap(sample_direction)).rgb * cos(theta) * sin(theta);
			
			n_samples++;
		}
	}

	irradiance *= PI / n_samples;

	return float4(irradiance, 1.0);
}

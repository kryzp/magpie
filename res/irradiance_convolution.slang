#include "util.slang"
#include "bindless.slang"

struct Arguments {
	float4x4 *transform_matrices;
	float3 *vertices;
	uint environment_map_id;
	uint linear_sampler_id;
};

[vk::push_constant]
Arguments args;

struct VS_Output {
	float4 sv_position : SV_Position;
	[vk::location(0)] float3 position;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint view_id : SV_ViewID)
{
	VS_Output output;
	output.position = args.vertices[vertex_id];
	output.sv_position = mul(args.transform_matrices[view_id], float4(output.position, 1.0));

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input)
{
    TextureCube env_map = bindless_cubes[args.environment_map_id];
    SamplerState sampler = bindless_samplers[args.linear_sampler_id];

    float3 irradiance = 0.0;

    float3 forward = normalize(input.position);
    float3 right = normalize(cross(forward, float3(0.0, 0.0, 1.0)));
    float3 up = normalize(cross(right, forward));

    float dt = 0.025;
    float n_samples = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += dt) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += dt) {
            float3 sample = float3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );

            float3x3 tangent_to_world = float3x3(right, up, forward);

            float3 sample_direction = mul(tangent_to_world, sample);

            sample_direction = WorldToCubemap(sample_direction);

            irradiance += env_map.Sample(sampler, sample_direction).rgb * cos(theta) * sin(theta);

			n_samples++;
		}
	}

	irradiance *= PI / n_samples;

	return float4(irradiance, 1.0);
}

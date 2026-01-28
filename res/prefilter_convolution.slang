#include "util.slang"
#include "bindless.slang"

struct Arguments {
	float4x4 *transform_matrices;
	float3 *vertices;
    uint environment_map_id;
    uint linear_sampler_id;
    float roughness;
};

[vk::push_constant]
Arguments args;

const static int SAMPLE_COUNT = 1024;

struct VS_Output {
	float4 sv_position : SV_Position;
	[vk::location(0)] float3 position;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint view_id : SV_ViewID)
{
	VS_Output output;
	output.position = args.vertices[vertex_id];
	output.sv_position = mul(args.transform_matrices[view_id], float4(output.position, 1.));

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	uint env_width, env_height;
	bindless_cubes[args.environment_map_id].GetDimensions(env_width, env_height);
	
	float3 normal = normalize(input.position);
	float3 R = normal;
	float3 V = R;
	
	float total_weight = 0.;
	
	float3 result = 0.;
	
	for (int i = 0; i < SAMPLE_COUNT; i++) {
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, normal, args.roughness);
		float3 L = normalize(2.*dot(V, H)*H - V);
		
		float NdotL = max(0., dot(normal, L));

		if (NdotL > 0.) {
			float NdotH = max(0., dot(normal, H));
			float HdotV = max(0., dot(H, V));
			
			float D = DistributionGGX(NdotH, args.roughness);
			
			float pdf = (D * NdotH / (4. * HdotV)) + .1;
			
			float sa_texel = 4. * PI / (6. * float(env_width) * float(env_height));
			float sa_sample = 1. / (float(SAMPLE_COUNT) * pdf + 0.1);

			float mip_level = args.roughness <= 0.1 ? 0. : .5 * log2(sa_sample / sa_texel);

			result += bindless_cubes[args.environment_map_id].SampleLevel(bindless_samplers[args.linear_sampler_id], WorldToCubemap(L), mip_level).rgb * NdotL;
			total_weight += NdotL;
		}
	}
	
	result /= total_weight;
	
	return float4(result, 1.);
}

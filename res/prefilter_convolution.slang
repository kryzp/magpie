#include "util.slang"

struct Arguments
{
	float4x4 *transform_matrices;
	float roughness;
	uint environment_map_id;
	uint linear_sampler_id;
	uint _padding;
};

[[vk::push_constant]]
Arguments args;

[[vk::binding(0)]] SamplerState bindless_samplers[];
[[vk::binding(2)]] TextureCube bindless_cubes[];

const static int SAMPLE_COUNT = 1024;

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
float4 FragmentMain(VS_Output input) : SV_Target
{
    uint env_width, env_height;
    bindless_cubes[args.environment_map_id].GetDimensions(env_width, env_height);
	
	float3 normal = normalize(input.position);
	float3 R = normal;
	float3 V = R;
	
	float total_weight = 0.0;
	
	float3 result = 0.0;
	
	for(int i = 0; i < SAMPLE_COUNT; i++)
	{
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, normal, args.roughness);
		float3 L = normalize(2.0*dot(V, H)*H - V);
		
		float NdotL = max(0.0, dot(normal, L));

        if(NdotL > 0.0)
        {
			float NdotH = max(0.0, dot(normal, H));
			float HdotV = max(0.0, dot(H, V));
			
			float D = DistributionGGX(NdotH, args.roughness);
			
			float pdf = (D * NdotH / (4.0 * HdotV)) + 0.01;
			
			float sa_texel = 4.0 * PI / (6.0 * float(env_width) * float(env_height));
			float sa_sample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.01);

			float mip_level = args.roughness <= 0.1 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);

            result += bindless_cubes[args.environment_map_id].SampleLevel(bindless_samplers[args.linear_sampler_id], WorldToCubemap(L), mip_level).rgb * NdotL;
			total_weight += NdotL;
		}
	}
	
	result /= total_weight;
	
	return float4(result, 1.0);
}

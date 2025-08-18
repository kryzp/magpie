#include "util.slang"

struct EnvironmentMapVertex
{
    [[vk::location(0)]] float3 position;
};

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

[[vk::binding(0)]] TextureCube bindless_images[];
[[vk::binding(1)]] SamplerState bindless_samplers[];

const static int SAMPLE_COUNT = 1024;

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

float RadicalInverse_VdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 normal, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	
	float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
	
	float3 H = float3(
		cos(phi) * sin_theta,
		sin(phi) * sin_theta,
		cos_theta
	);
	
	float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, normal));
	float3 bitangent = cross(normal, tangent);
	
	float3x3 inverse_tbn = transpose(float3x3(tangent, bitangent, normal));
	float3 sample_vector = mul(inverse_tbn, H);
	
	return normalize(sample_vector);
}

float DistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;

	return a2 / (denom * denom * PI);
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
    uint env_width, env_height;
    bindless_images[args.environment_map_id].GetDimensions(env_width, env_height);
	
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
			
			float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;
			
			float sa_texel = 4.0 * PI / (6.0 * float(env_width) * float(env_height));
			float sa_sample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

			float mip_level = args.roughness <= 0.001 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);

            result += bindless_images[args.environment_map_id].SampleLevel(bindless_samplers[args.linear_sampler_id], WorldToCubemap(L), mip_level).rgb * NdotL;
			total_weight += NdotL;
		}
	}
	
	result /= total_weight;
	
	return float4(result, 1.0);
}

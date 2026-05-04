import bindless;
import math;
import sampling;
import lighting;
import ibl_convolution;

struct PushConstants
{
	float4x4 *transform_matrices;
	float3 *vertices;
	uint environment_map_id;
	uint linear_sampler_id;
	float roughness;
};

[vk::push_constant] PushConstants args;

const static int SAMPLE_COUNT = 1024;

[shader("vertex")]
ibl_convolution::VS_Output VertexMain(uint vertex_id : SV_VertexID, uint view_id : SV_ViewID)
{
	return ibl_convolution::CubemapFaceVS(vertex_id, view_id, args.transform_matrices, args.vertices);
}

[shader("fragment")]
float4 FragmentMain(ibl_convolution::VS_Output input) : SV_Target
{
	TextureCube environment_map = bdl::cubes[args.environment_map_id];
	SamplerState sampler = bdl::samplers[args.linear_sampler_id];

	float roughness = args.roughness;

	uint env_width, env_height;
	environment_map.GetDimensions(env_width, env_height);

	float3 normal = normalize(input.position);
	float3 R = normal;
	float3 V = R;

	float total_weight = 0.0;
	float3 result = 0.0;

	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		float2 Xi = sampling::Hammersley(i, SAMPLE_COUNT);
		float3 H = sampling::ImportanceSampleGGX(Xi, normal, roughness);
		float3 L = normalize(2. * dot(V, H) * H - V);

		float NdotL = max(0.0, dot(normal, L));
		float NdotH = max(0.0, dot(normal, H));
		float HdotV = max(0.0, dot(H, V));

		if (NdotL > 0.0)
		{
			float D = lighting::DistributionGGX(NdotH, roughness);

			float pdf = ((D * NdotH) / (4.0 * HdotV)) + 0.0001;

			float sa_texel = 4.0 * math::PI / (6.0 * float(env_width) * float(env_height));
			float sa_sample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

			float mip_level = roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel);

			result += environment_map.SampleLevel(
				sampler,
				math::WorldToCubemap(L),
				mip_level
			).rgb * NdotL;

			total_weight += NdotL;
		}
	}

	result /= total_weight;

	return float4(result, 1.0);
}

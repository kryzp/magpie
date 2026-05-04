import bindless;
import frame;
import scene;
import lighting;
import shadows;

struct PushConstants
{
	frame::FrameData *frame_data;
	lighting::Light *lights;
	float3 *vertices;
	shadows::ShadowCaster *shadow_casters;

	uint position;
	uint albedo;
	uint normal;
	uint emissive;
	uint material;

	uint linear_sampler;
	uint shadow_sampler;
};

[vk::push_constant] PushConstants args;

struct VS_Output
{
	float4 sv_position : SV_Position;
	[vk::location(0)] uint light_id;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint base_instance : SV_InstanceID)
{
	frame::FrameData frame_data = *args.frame_data;
	lighting::Light light = args.lights[base_instance];
	float3 position = args.vertices[vertex_id];

	VS_Output output;
	output.sv_position = mul(frame_data.view_projection, mul(light.transform, float4(position, 1.0)));
	output.light_id = base_instance;

	return output;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	SamplerState sampler = bdl::samplers[args.linear_sampler];
	float2 uv = input.sv_position.xy / args.frame_data.window_resolution.xy;

	float3 position    = bdl::textures[args.position]    .Sample(sampler, uv).rgb;
	float3 albedo      = bdl::textures[args.albedo]      .Sample(sampler, uv).rgb;
	float3 material    = bdl::textures[args.material]    .Sample(sampler, uv).rgb;
	float3 normal      = bdl::textures[args.normal]      .Sample(sampler, uv).rgb;
	float3 emissive    = bdl::textures[args.emissive]    .Sample(sampler, uv).rgb;

	lighting::Light light = args.lights[input.light_id];

	float roughness_value = material.g;
	float metallic_value = material.b;

	float3 F0 = lighting::ComputeF0(albedo, metallic_value);

	float3 view_dir = normalize(args.frame_data.camera_position - position);
	float NdotV = max(0.0, dot(normal, view_dir));

	float3 delta_x = light.position - position;
	float distance_squared = dot(delta_x, delta_x);

	float attenuation = lighting::CalculateAttenuation(distance_squared, light.attenuation);
	float3 radiance = light.colour * light.intensity * attenuation;

	float3 light_dir = normalize(delta_x);
	float3 halfway_dir = normalize(light_dir + view_dir);

	float NdotL = max(0.0, dot(normal, light_dir));
	float NdotH = max(0.0, dot(normal, halfway_dir));
	float VdotH = max(0.0, dot(halfway_dir, view_dir));

	float3 F = lighting::FresnelSchlick(VdotH, F0, 0.0);
	float NDF = lighting::DistributionGGX(NdotH, roughness_value);
	float G = lighting::GeometrySmith_ARTISTIC(NdotV, NdotL, roughness_value);

	float3 kD = (1.0 - F) * (1.0 - metallic_value);

	float3 diffuse = albedo;
	float3 specular = (F * G * NDF) / (4.0 * NdotL * NdotV + 0.0001);

	float shadow_factor = 1.0;

	if (light.shadow_slot_index != -1)
	{
		shadows::ShadowCaster caster = args.shadow_casters[light.shadow_slot_index];
		shadow_factor = shadows::CalculatePointShadow(position, light, caster, *args.frame_data, args.shadow_sampler);
	}

	float3 Lo = radiance * NdotL * (kD * diffuse + specular) * shadow_factor;

	return float4(Lo, 1.0);
}

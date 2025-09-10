#include "util.slang"
#include "bindless.slang"

struct Light {
	float4 position;
	float4 colour;
	float4 attenuation;
	float4x4 transform;
};

struct PushConstants {
	FrameData *frame_data;
	Light *lights;

	uint position;
	uint albedo;
	uint normal;
	uint material;
	uint emissive;

	uint linear_sampler;

	uint _padding[2];
};

[vk::push_constant]
PushConstants args;

struct VS_Output {
	float4 sv_position : SV_Position;
	uint light_id;
};

[shader("vertex")]
VS_Output VertexMain(float3 position, uint base_instance : SV_InstanceID)
{
	FrameData *frame_data = args.frame_data;
	Light light = args.lights[base_instance];

	VS_Output output;
	
	output.sv_position = mul(frame_data->projection, mul(frame_data->view, mul(light.transform, float4(position, 1.))));
	
	output.light_id = base_instance;

	return output;
}

float CalculateAttenuation(float distance_squared, float3 params)
{
	return 1. / dot(float3(distance_squared, sqrt(distance_squared), 1.), params);
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	SamplerState sampler = bindless_samplers[args.linear_sampler];
	float2 uv = input.sv_position.xy / args.frame_data->window_resolution.xy;

	float3 position    = bindless_images[args.position]    .Sample(sampler, uv).rgb;
	float3 albedo      = bindless_images[args.albedo]      .Sample(sampler, uv).rgb;
	float3 material    = bindless_images[args.material]    .Sample(sampler, uv).rgb;
	float3 normal      = bindless_images[args.normal]      .Sample(sampler, uv).rgb;
	float3 emissive    = bindless_images[args.emissive]    .Sample(sampler, uv).rgb;

	normal = 2.*normal - 1.;

	Light light = args.lights[input.light_id];

	float roughness_value = material.g;
	float metallic_value = material.b;

	float3 F0 = lerp(.04, albedo, metallic_value);

	float3 view_dir = normalize(args.frame_data->camera_position.xyz - position);

	float NdotV = max(0., dot(normal, view_dir));

	float3 delta_x = light.position.xyz - position;
	float distance_squared = dot(delta_x, delta_x);

	float attenuation = CalculateAttenuation(distance_squared, light.attenuation.xyz);
	float intensity = light.colour.a;

	float3 radiance = light.colour.rgb * intensity * attenuation;

	float3 light_dir = normalize(delta_x);
	float3 halfway_dir = normalize(light_dir + view_dir);

	float NdotL = max(0., dot(normal, light_dir));
	float NdotH = max(0., dot(normal, halfway_dir));
	float VdotH = max(0., dot(halfway_dir, view_dir));

	float3 F = FresnelSchlick(VdotH, F0, 0.0);
	float NDF = DistributionGGX(NdotH, roughness_value);
	float G = GeometrySmith(NdotV, NdotL, roughness_value);

	float3 kD = (1. - F) * (1. - metallic_value);

	float3 diffuse = albedo;
	float3 specular = (F * G * NDF) / (4. * NdotL * NdotV + .0001);

	float3 Lo = radiance * NdotL * (kD * diffuse + specular);

	return float4(Lo, 1.);
}

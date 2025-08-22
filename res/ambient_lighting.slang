#include "util.slang"

struct PushConstants
{
	FrameData *frame_data;

	uint position;
	uint albedo;
	uint normal;
	uint material;
	uint emissive;

	uint irradiance_map;
	uint prefilter_map;
	uint brdf_lut;

	uint linear_sampler;

	uint _padding[3];
};

[[vk::push_constant]]
PushConstants args;

[[vk::binding(0)]] SamplerState bindless_samplers[];
[[vk::binding(1)]] Texture2D bindless_images[];
[[vk::binding(2)]] TextureCube bindless_cubes[];

struct VS_Output
{
    float4 sv_position : SV_Position;

    [[vk::location(0)]] float2 uv;
};

[shader("vertex")]
VS_Output VertexMain(uint id : SV_VertexID)
{
    VS_Output output;
    output.uv = float2((id << 1) & 2, id & 2);
    output.sv_position = float4(output.uv.x*2. - 1., 1. - 2.*output.uv.y, 0., 1.);

    return output;
}

#define MAX_REFLECTION_LOD 4.

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	SamplerState sampler = bindless_samplers[args.linear_sampler];
	float2 uv = input.uv;

	float3 position    = bindless_images[args.position]    .Sample(sampler, uv).rgb;
	float3 albedo      = bindless_images[args.albedo]      .Sample(sampler, uv).rgb;
	float3 material    = bindless_images[args.material]    .Sample(sampler, uv).rgb;
	float3 normal      = bindless_images[args.normal]      .Sample(sampler, uv).rgb;
	float3 emissive    = bindless_images[args.emissive]    .Sample(sampler, uv).rgb;

	normal = 2.*normal - 1.;

	float ambient_occlusion = 1.;
	float roughness_value = material.g;
	float metallic_value = material.b;

	float3 F0 = lerp(0.4, albedo, metallic_value);

	float3 view_dir = normalize(args.frame_data->camera_position.xyz - position);

	float NdotV = max(0., dot(normal, view_dir));

	float3 F = FresnelSchlick(NdotV, F0, roughness_value);
	float3 kD = (1. - F) * (1. - metallic_value);

	float3 reflected = reflect(-view_dir, normal);

	float prefilter_mip_level = roughness_value * MAX_REFLECTION_LOD;
	float3 prefiltered_colour = bindless_cubes[args.prefilter_map].SampleLevel(sampler, WorldToCubemap(reflected), prefilter_mip_level).rgb;

	float2 environment_brdf = bindless_images[args.brdf_lut].Sample(sampler, float2(NdotV, roughness_value)).xy;

	float3 specular = prefiltered_colour * (F * environment_brdf.x + environment_brdf.y);

    float3 irradiance = bindless_cubes[args.irradiance_map].Sample(sampler, WorldToCubemap(normal)).rgb;
	float3 diffuse = irradiance * albedo;
	float3 ambient = (kD * diffuse + specular) * ambient_occlusion;

	float3 final_colour = ambient + emissive;

    return float4(final_colour, 1.);
}

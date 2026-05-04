import bindless;
import frame;
import lighting;
import irradiance;
import math;
import fullscreen;

struct PushConstants
{
	frame::FrameData *frame_data;

	irradiance::ProbeSH *irr_sh_buffer;
	irradiance::ProbeGridInfo *irr_grid_info_buffer;

	uint position;
	uint albedo;
	uint normal;
	uint material;
	uint emissive;

	uint irradiance_fallback_cubemap;
	uint prefilter_cubemap;
	uint brdf_lut;

	uint linear_sampler;
};

[vk::push_constant]
PushConstants args;

[shader("vertex")]
fullscreen::VS_Output VertexMain(uint id : SV_VertexID)
{
	return fullscreen::FullscreenVS(id);
}

[shader("fragment")]
float4 FragmentMain(fullscreen::VS_Output input) : SV_Target
{
	SamplerState sampler = bdl::samplers[args.linear_sampler];
	float2 uv = input.uv;

	float3 position    = bdl::textures[args.position]   .Sample(sampler, uv).rgb;
	float3 albedo      = bdl::textures[args.albedo]     .Sample(sampler, uv).rgb;
	float3 normal      = bdl::textures[args.normal]     .Sample(sampler, uv).rgb;
	float3 material    = bdl::textures[args.material]   .Sample(sampler, uv).rgb;
	float3 emissive    = bdl::textures[args.emissive]   .Sample(sampler, uv).rgb;

	TextureCube irradiance_fallback_cubemap = bdl::cubes[args.irradiance_fallback_cubemap];
	TextureCube prefilter_cubemap = bdl::cubes[args.prefilter_cubemap];

	float ambient_occlusion = 1.0;
	float roughness_value = material.g;
	float metallic_value = material.b;

	float3 F0 = lighting::ComputeF0(albedo, metallic_value);

	float3 view_dir = normalize(args.frame_data.camera_position.xyz - position);
	float NdotV = max(0., dot(normal, view_dir));

	float3 F = F0; // lighting::FresnelSchlick(NdotV, F0, roughness_value);
	float3 kD = (1. - F) * (1.0 - metallic_value);

	float3 reflected = normalize(reflect(-view_dir, normal));

	float prefilter_mip_level = roughness_value * irradiance::MAX_REFLECTION_LOD;
	float3 prefiltered_colour = prefilter_cubemap.SampleLevel(sampler, math::WorldToCubemap(reflected), prefilter_mip_level).rgb;

	float2 environment_brdf = bdl::textures[args.brdf_lut].Sample(sampler, float2(NdotV, roughness_value)).xy;

	float3 specular = prefiltered_colour * (F * environment_brdf.x + environment_brdf.y);

	float3 irr = 0.0;

	if (args.irr_sh_buffer)
		irr = irradiance::SampleIrradianceGrid(position, normal, *args.irr_grid_info_buffer, args.irr_sh_buffer);
	else
		irr = irradiance_fallback_cubemap.Sample(sampler, math::WorldToCubemap(normal)).rgb;

	float3 diffuse = irr * albedo;
	float3 ambient = (kD * diffuse + specular) * ambient_occlusion;

	float3 final_colour = ambient + emissive;

	return float4(final_colour, 1.0);
}

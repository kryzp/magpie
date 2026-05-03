import util;
import bindless;
import types;
import irradiance;

struct PushConstants {
	types::FrameData *frame_data;

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

struct VS_Output {
    float4 sv_position : SV_Position;
    [vk::location(0)] float2 uv;
};

[shader("vertex")]
VS_Output VertexMain(uint id : SV_VertexID)
{
    VS_Output output;
    output.uv = float2(id & 2, (id << 1) & 2);
    output.sv_position = float4(output.uv.x*2.0 - 1.0, 1.0 - 2.0*output.uv.y, 0.0, 1.0);

    return output;
}

#define MAX_REFLECTION_LOD 5.0

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
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

	float3 F0 = lerp(0.04, albedo, metallic_value);

	float3 view_dir = normalize(args.frame_data.camera_position.xyz - position);

	float NdotV = max(0., dot(normal, view_dir));

	float3 F = F0;//util::FresnelSchlick(NdotV, F0, roughness_value);
	float3 kD = (1. - F) * (1.0 - metallic_value);

	float3 reflected = normalize(reflect(-view_dir, normal));

    float prefilter_mip_level = roughness_value * MAX_REFLECTION_LOD;
    float3 prefiltered_colour = prefilter_cubemap.SampleLevel(sampler, util::WorldToCubemap(reflected), prefilter_mip_level).rgb;

	prefiltered_colour = 0.0;
	
    float2 environment_brdf = bdl::textures[args.brdf_lut].Sample(sampler, float2(NdotV, roughness_value)).xy;

	float3 specular = prefiltered_colour * (F * environment_brdf.x + environment_brdf.y);

	float3 irr = 0.0;
	
	if (args.irr_sh_buffer)
		irr = irradiance::SampleIrradianceGrid(position, normal, *args.irr_grid_info_buffer, args.irr_sh_buffer);
	else
		irr = irradiance_fallback_cubemap.Sample(sampler, util::WorldToCubemap(normal)).rgb;		

	float3 diffuse = irr * albedo;
	float3 ambient = (kD * diffuse + specular) * ambient_occlusion;

    float3 final_colour = ambient + emissive;

    return float4(final_colour, 1.0);
}

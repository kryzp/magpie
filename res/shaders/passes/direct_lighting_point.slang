import util;
import bindless;
import types;

struct PushConstants {
	types::FrameData *frame_data;
	types::Light *lights;
	float3 *vertices;
	types::ShadowCaster *shadow_casters;

	uint position;
	uint albedo;
	uint normal;
	uint emissive;
	uint material;

	uint linear_sampler;
	uint shadow_sampler;
};

[vk::push_constant]
PushConstants args;

struct VS_Output {
	float4 sv_position : SV_Position;
	[vk::location(0)] uint light_id;
};

[shader("vertex")]
VS_Output VertexMain(uint vertex_id : SV_VertexID, uint base_instance : SV_InstanceID)
{
	types::FrameData frame_data = *args.frame_data;
	types::Light light = args.lights[base_instance];
	float3 position = args.vertices[vertex_id];

    VS_Output output;
    output.sv_position = mul(frame_data.view_projection, mul(light.transform, float4(position, 1.0)));
	output.light_id = base_instance;

	return output;
}

float CalculateAttenuation(float distance_squared, float3 params)
{
	return 1.0 / dot(float3(distance_squared, sqrt(distance_squared), 1.0), params);
}

static const float3 pcf_offsets[20] = {
	float3( 1.0, 1.0, 1.0), float3( 1.0,-1.0, 1.0), float3(-1.0, -1.0,  1.0), float3(-1.0, 1.0, 1.0), 
	float3( 1.0, 1.0,-1.0), float3( 1.0,-1.0,-1.0), float3(-1.0, -1.0, -1.0), float3(-1.0, 1.0,-1.0),
	float3( 1.0, 1.0, 0.0), float3( 1.0,-1.0, 0.0), float3(-1.0, -1.0,  0.0), float3(-1.0, 1.0, 0.0),
	float3( 1.0, 0.0, 1.0), float3(-1.0, 0.0, 1.0), float3( 1.0,  0.0, -1.0), float3(-1.0, 0.0,-1.0),
	float3( 0.0, 1.0, 1.0), float3( 0.0,-1.0, 1.0), float3( 0.0, -1.0, -1.0), float3( 0.0, 1.0,-1.0)
};

float CalculateShadow(float3 frag_position, in types::Light light)
{
    types::ShadowCaster caster = args.shadow_casters[light.shadow_slot_index];

    SamplerState shadow_sampler = bindless::samplers[args.shadow_sampler];
    TextureCube shadow_map = bindless::cubes[caster.shadow_map];

    float3 light_to_frag = frag_position - light.position;    
	float current_depth = length(light_to_frag);

    float shadow = 0.0;

    float bias = 0.05;
    int sample_count = 20;

    float disk_radius = (1.0 + length(light.position - args.frame_data.camera_position)*10.0) * 0.0005;

    for (int i = 0; i < sample_count; i++) {
        float3 sample_dir = light_to_frag + pcf_offsets[i] * disk_radius;
        sample_dir = normalize(sample_dir);

        float sampled_depth = shadow_map.Sample(shadow_sampler, util::WorldToCubemap(sample_dir)).r;

        sampled_depth = caster.near_plane + sampled_depth * (caster.far_plane - caster.near_plane);

        if (current_depth - bias > sampled_depth)
            shadow += 1.0;
    }

    shadow /= float(sample_count);

    return 1.0 - shadow;
}

[shader("fragment")]
float4 FragmentMain(VS_Output input) : SV_Target
{
	SamplerState sampler = bindless::samplers[args.linear_sampler];
	float2 uv = input.sv_position.xy / args.frame_data.window_resolution.xy;

	float3 position    = bindless::images[args.position]    .Sample(sampler, uv).rgb;
	float3 albedo      = bindless::images[args.albedo]      .Sample(sampler, uv).rgb;
	float3 material    = bindless::images[args.material]    .Sample(sampler, uv).rgb;
	float3 normal      = bindless::images[args.normal]      .Sample(sampler, uv).rgb;
	float3 emissive    = bindless::images[args.emissive]    .Sample(sampler, uv).rgb;

	types::Light light = args.lights[input.light_id];

	float roughness_value = material.g;
	float metallic_value = material.b;

	float3 F0 = lerp(0.04, albedo, metallic_value);

	float3 view_dir = normalize(args.frame_data.camera_position - position);

	float NdotV = max(0.0, dot(normal, view_dir));

	float3 delta_x = light.position - position;
	float distance_squared = dot(delta_x, delta_x);

	float attenuation = CalculateAttenuation(distance_squared, light.attenuation);

	float3 radiance = light.colour * light.intensity * attenuation;

	float3 light_dir = normalize(delta_x);
	float3 halfway_dir = normalize(light_dir + view_dir);

	float NdotL = max(0.0, dot(normal, light_dir));
	float NdotH = max(0.0, dot(normal, halfway_dir));
	float VdotH = max(0.0, dot(halfway_dir, view_dir));

	float3 F = util::FresnelSchlick(VdotH, F0, 0.0);
	float NDF = util::DistributionGGX(NdotH, roughness_value);
	float G = util::GeometrySmith_ARTISTIC(NdotV, NdotL, roughness_value);

	float3 kD = (1.0 - F) * (1.0 - metallic_value);

	float3 diffuse = albedo;
	float3 specular = (F * G * NDF) / (4.0 * NdotL * NdotV + 0.0001);

    float shadow_factor = 1.0;

    if (light.shadow_slot_index != -1)
        shadow_factor = CalculateShadow(position, light);

	float3 Lo = radiance * NdotL * (kD * diffuse + specular) * shadow_factor;

	return float4(Lo, 1.0);
}

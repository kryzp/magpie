#include "shared/vertices.slang"
#include "shared/bindless.slang"
#include "shared/types.slang"
#include "shared/pbr.slang"

struct PushConstants
{
    FrameData *frameData;
    PointLight *lights;

    float4x4 model;

    uint position_id;
    uint albedo_id;
    uint normal_id;
    uint material_id;
    uint emissive_id;

    uint brdfLUT_id;

    uint textureSampler_id;

    uint light_id;
};

[[vk::push_constant]]
PushConstants pc;

struct VS_Output
{
	float4 sv_position : SV_Position;
};

[shader("vertex")]
VS_Output vertexMain(PrimitiveVertex vertex)
{
	VS_Output output;
    output.sv_position = mul(pc->frameData->proj, mul(pc->frameData->view, mul(pc.model, float4(vertex.position, 1.0))));

	return output;
}

float calculateAttenuation(float distanceSquared, float3 params)
{
	return 1.0 / dot(float3(distanceSquared, sqrt(distanceSquared), 1.0), params);
}

[shader("fragment")]
float4 fragmentMain(VS_Output input) : SV_Target
{
    PointLight light = pc.lights[pc.light_id];

    SamplerState sampler = g_bindlessSamplers[pc.textureSampler_id];

    float2 uv = input.sv_position.xy / float2(1280.0, 720.0);

    float3 position = g_bindlessTexture2D[pc.position_id].Sample(sampler, uv).xyz;
    float3 albedo = g_bindlessTexture2D[pc.albedo_id].Sample(sampler, uv).rgb;
    float3 material = g_bindlessTexture2D[pc.material_id].Sample(sampler, uv).rgb;
    float3 normal = g_bindlessTexture2D[pc.normal_id].Sample(sampler, uv).rgb;
    float3 emissive = g_bindlessTexture2D[pc.emissive_id].Sample(sampler, uv).rgb;

    normal = normal * 2.0 - 1.0;

    float roughnessValue = material.g;
    float metallicValue = material.b;

    float3 F0 = lerp(0.04, albedo, metallicValue);

    float3 viewDir = normalize(pc.frameData->cameraPosition.xyz - position);

    float NdotV = max(0.0, dot(normal, viewDir));

    float3 deltaX = light.position.xyz - position;
	float distanceSquared = dot(deltaX, deltaX);

	float attenuation = calculateAttenuation(distanceSquared, light.attenuation.xyz);
	float intensity = light.colour.a;

	float3 radiance = light.colour.rgb * intensity * attenuation;

	float3 lightDir = normalize(deltaX);
	float3 halfwayDir = normalize(lightDir + viewDir);

	float NdotL = max(0.0, dot(normal, lightDir));
	float NdotH = max(0.0, dot(normal, halfwayDir));

	float VdotH = max(0.0, dot(halfwayDir, viewDir));

	float3 F = fresnelSchlick(VdotH, F0, 0.0);

	float NDF = distributionGGX(NdotH, roughnessValue);
	float G = geometrySmith(NdotV, NdotL, roughnessValue);

	float3 kD = (1.0 - F) * (1.0 - metallicValue);

	float3 diffuse = albedo;
	float3 specular = (F * G * NDF) / (4.0 * NdotL * NdotV + 0.0001);

    float3 Lo = radiance * NdotL * (kD * diffuse + specular);

	return float4(Lo, 1.0);
}

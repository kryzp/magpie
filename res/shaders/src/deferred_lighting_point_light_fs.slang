#include "primitive_vs.slang"

#include "shared/bindless.slang"
#include "shared/pbr.slang"

struct PushConstants
{
    uint position_id;
    uint albedo_id;
    uint normal_id;
    uint material_id;
    uint emissive_id;

    uint irradianceMap_id;
    uint prefilterMap_id;
    uint brdfLUT_id;

    uint textureSampler_id;

    uint _padding[2];

    float4 cameraPosition;
};

[[vk::push_constant]]
PushConstants pc;

float calculateAttenuation(float distanceSquared, float3 params)
{
	return 1.0 / dot(float3(distanceSquared, sqrt(distanceSquared), 1.0), params);
}

[shader("fragment")]
float4 fragmentMain(VS_Output input) : SV_Target
{
	SamplerState sampler = g_bindlessSamplers[pc.textureSampler_id];

	float3 position		= g_bindlessTexture2D[pc.position_id]	.Sample(sampler, uv).xyz;
	float3 albedo		= g_bindlessTexture2D[pc.albedo_id]		.Sample(sampler, uv).rgb;
	float3 pbrTexture	= g_bindlessTexture2D[pc.material_id]	.Sample(sampler, uv).rgb;
	float3 normal		= g_bindlessTexture2D[pc.normal_id]		.Sample(sampler, uv).rgb;
	float3 emissive		= g_bindlessTexture2D[pc.emissive_id]	.Sample(sampler, uv).rgb;
	
	float3 viewDir = normalize(pc.cameraPosition.xyz - input.position.xyz);

	float3 deltaX = light.position.xyz - input.position.xyz;
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

	/*
	float isShadowCaster = light.attenuation.w;

	float shadow = (isShadowCaster >= 0.99)
		? calculateShadow(input.position, light.lightSpaceMatrix, lightDir, geomNormal, light.atlasRegion)
		: 0.0;
	*/

	float shadow = 0.0;

	return float4(radiance * NdotL * (kD * diffuse + specular) * (1.0 - shadow), 1.0);
}

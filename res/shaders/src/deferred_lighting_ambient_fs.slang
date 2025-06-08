#include "fullscreen_triangle_vs.slang"

#include "shared/bindless.slang"

#define PI 3.14159265
#define MAX_REFLECTION_LOD 4.0

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
	uint cubemapSampler_id;

    uint _padding[2];

	float4 cameraPosition;
};

[[vk::push_constant]]
PushConstants pc;

float3 fresnelSchlick(float VdotH, float3 F0, float roughness)
{
	float t = pow(1.0 - max(VdotH, 0.0), 5.0);
	float3 F90 = max(1.0 - roughness, F0);
	return lerp(F0, F90, t);
}

float distributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;

	return a2 / max(0.0000001, denom * denom * PI);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;

	return NdotV / lerp(k, 1.0, NdotV);
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
	float ggx2 = geometrySchlickGGX(NdotV, roughness);
	float ggx1 = geometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

[shader("fragment")]
float4 fragmentMain(VS_Output input) : SV_Target
{
	SamplerState textureSampler = g_bindlessSamplers[pc.textureSampler_id];
	SamplerState cubemapSampler = g_bindlessSamplers[pc.cubemapSampler_id];

	float2 uv = input.uv;

	float3 position				= g_bindlessTexture2D[pc.position_id]		.Sample(textureSampler, uv).xyz;
	float3 albedo               = g_bindlessTexture2D[pc.albedo_id]			.Sample(textureSampler, uv).rgb;
	float3 pbrTexture           = g_bindlessTexture2D[pc.material_id]       .Sample(textureSampler, uv).rgb;
	float3 normal               = g_bindlessTexture2D[pc.normal_id]         .Sample(textureSampler, uv).rgb;
	float3 emissive             = g_bindlessTexture2D[pc.emissive_id]       .Sample(textureSampler, uv).rgb;

	normal = 2.0*normal - 1.0;

	float ambientOcclusion = 1.0;

	float roughnessValue = pbrTexture.g;
	float metallicValue = pbrTexture.b;

	float3 F0 = lerp(0.04, albedo, metallicValue);

	float3 viewDir = normalize(pc.cameraPosition.xyz - position);

	float NdotV = max(0.0, dot(normal, viewDir));

	float3 F = fresnelSchlick(NdotV, F0, roughnessValue);
	float3 kD = (1.0 - F) * (1.0 - metallicValue);

	float3 reflected = reflect(-viewDir, normal);

	float prefilterMipLevel = roughnessValue * MAX_REFLECTION_LOD;
	float3 prefilteredColour = g_bindlessTextureCube[pc.prefilterMap_id].SampleLevel(cubemapSampler, reflected, prefilterMipLevel).rgb;

	float2 environmentBRDF = g_bindlessTexture2D[pc.brdfLUT_id].Sample(textureSampler, float2(NdotV, roughnessValue)).xy;

	float3 specular = prefilteredColour * (F * environmentBRDF.x + environmentBRDF.y);

	float3 irradiance = g_bindlessTextureCube[pc.irradianceMap_id].Sample(cubemapSampler, normal).rgb;
	float3 diffuse = irradiance * albedo;
	float3 ambient = (kD * diffuse + specular) * ambientOcclusion;

	float3 finalColour = ambient + emissive;

    return float4(finalColour, 1.0);
}

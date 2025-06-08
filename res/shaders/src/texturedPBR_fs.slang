#include "model_vs.slang"

#include "shared/bindless.slang"
#include "shared/types.slang"

#define PI 3.14159265

#define MAX_REFLECTION_LOD 4.0
#define MAX_POINT_LIGHTS 16
#define SHADOW_BIAS 0.01

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

/*
float calculateShadow(float4 position, float4x4 lightSpaceMatrix, float3 lightDirection, float3 geometryNormal, float4 region)
{
	float NdotL = dot(lightDirection, geometryNormal);
	float tanT = sqrt(1.0 - NdotL * NdotL) / NdotL;

	position.xyz += geometryNormal * tanT * SHADOW_BIAS;

	float4 lightSpacePosition = mul(lightSpaceMatrix, position);

	float3 projCoords = lightSpacePosition.xyz / lightSpacePosition.w;

	projCoords.xy = (projCoords.xy * 0.5) + 0.5;
	projCoords.y = 1.0 - projCoords.y;

	float2 uv = region.xy + projCoords.xy*region.zw;

	float depthHere = projCoords.z;
	float depthScreen = g_bindlessTexture2D[g_bindless.shadowAtlas_id].Sample(g_bindlessSamplers[g_bindless.shadowAtlasSampler_id], uv).x;

	return depthHere > depthScreen ? 1.0 : 0.0;
}
*/

float calculateAttenuation(float distanceSquared, float3 params)
{
	return 1.0 / dot(float3(distanceSquared, sqrt(distanceSquared), 1.0), params);
}

[shader("fragment")]
float4 fragmentMain(VS_Output input) : SV_Target
{
	float2 uv = frac(input.uv);

	SamplerState textureSampler = g_bindlessSamplers[g_bindless.textureSampler_id];
	SamplerState cubemapSampler = g_bindlessSamplers[g_bindless.cubemapSampler_id];

	FrameData frameData			= g_bindless.buffers.frameData[0];
	MaterialData materialData	= g_bindless.buffers.materials[g_bindless.material_id];

	float3 albedo					= g_bindlessTexture2D[materialData.diffuse_id]		.Sample(textureSampler, uv).rgb;
    float ambientOcclusion			= g_bindlessTexture2D[materialData.ambient_id]		.Sample(textureSampler, uv).r;
    float3 pbrTexture				= g_bindlessTexture2D[materialData.material_id]		.Sample(textureSampler, uv).rgb;
	float3 normal					= g_bindlessTexture2D[materialData.normal_id]		.Sample(textureSampler, uv).rgb;
	float3 emissive					= g_bindlessTexture2D[materialData.emissive_id]		.Sample(textureSampler, uv).rgb;
	
	// light testing white surface
//	albedo = 1.0;
//	ambientOcclusion				= g_bindlessTexture2D[materialData.ambient_id]							.Sample(textureSampler, uv).r;
//	ambientRoughnessMetallic		= float3(g_bindlessTexture2D[materialData.material_id]	.Sample(textureSampler, uv).r, 1.0, 0.0);
//	normal							= float3(0.5, 0.5, 1.0);
//	emissive						= 0.0;

    ambientOcclusion		+= pbrTexture.r;
    float roughnessValue	 = pbrTexture.g;
    float metallicValue		 = pbrTexture.b;
	
	float3 F0 = lerp(0.04, albedo, metallicValue);

	normal = normalize(mul(input.tbn, 2.0*normal - 1.0));
	float3 geomNormal = mul(input.tbn, float3(0.0, 0.0, 1.0));

	float3 viewDir = normalize(frameData.viewPosition.xyz - input.position.xyz);
	
	float NdotV = max(0.0, dot(normal, viewDir));
	
	float3 Lo = 0.0;

    for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		PointLight light = g_bindless.buffers.pointLights[i];

        if (dot(light.colour.xyz, light.colour.xyz) < 0.001)
            continue;

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
		float3 specular = (F * G * NDF) / (4.0*NdotL*NdotV + 0.0001);

		/*
        float isShadowCaster = light.attenuation.w;

        float shadow = (isShadowCaster >= 0.99)
			? calculateShadow(input.position, light.lightSpaceMatrix, lightDir, geomNormal, light.atlasRegion)
			: 0.0;
		*/

        float shadow = 0.0;

		Lo += radiance * NdotL * (kD * diffuse + specular) * (1.0 - shadow);
	}

	float3 F = fresnelSchlick(NdotV, F0, roughnessValue);
	float3 kD = (1.0 - F) * (1.0 - metallicValue);
	
	float3 reflected = reflect(-viewDir, normal);
	
	float prefilterMipLevel = roughnessValue * MAX_REFLECTION_LOD;
	float3 prefilteredColour = g_bindlessTextureCube[g_bindless.prefilterMap_id].SampleLevel(cubemapSampler, reflected, prefilterMipLevel).rgb;
	
	float2 environmentBRDF = g_bindlessTexture2D[g_bindless.brdfLUT_id].Sample(textureSampler, float2(NdotV, roughnessValue)).xy;
	
	float3 specular = prefilteredColour * (F * environmentBRDF.x + environmentBRDF.y);
	
	float3 irradiance = g_bindlessTextureCube[g_bindless.irradianceMap_id].Sample(cubemapSampler, normal).rgb;
	float3 diffuse = irradiance * albedo;
	float3 ambient = (kD * diffuse + specular) * ambientOcclusion;
	
	float3 finalColour = ambient + Lo + emissive;
	
	finalColour *= input.colour;
	
	return float4(finalColour, 1.0);
}

#include "bindless.hlsl"

#include "model_common.hlsl"

#define MAX_REFLECTION_LOD 4.0

struct PSInput
{
	[[vk::location(VS_MODEL_OUT_SLOT_COLOR)]]
    float3 colour : COLOR;

	[[vk::location(VS_MODEL_OUT_SLOT_POSITION)]]
    float4 position : TEXCOORD0;

	[[vk::location(VS_MODEL_OUT_SLOT_UV)]]
    float2 texCoord : TEXCOORD1;

	[[vk::location(VS_MODEL_OUT_SLOT_TBN)]]
    float3x3 tbn : TEXCOORD3;
};

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
	
	return a2 / max(0.0000001, denom * denom * MATH_PI);
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

#define SHADOW_BIAS_MIN 0.0001
#define SHADOW_BIAS_MAX 0.001

float calculateShadow(in float4 lightSpacePosition, float NdotL, float3 normal, float4 region)
{
    Texture2D shadowAtlas = texture2DTable[pc.shadowAtlas_ID];

    float3 modifiedLightPosition = lightSpacePosition.xyz;

    modifiedLightPosition += 0.025 * normal * NdotL;

    float3 projCoords = modifiedLightPosition / lightSpacePosition.w;

    projCoords.xy = (projCoords.xy * 0.5) + 0.5;
    projCoords.y = 1.0 - projCoords.y;

    float2 uv = region.xy + projCoords.xy*region.zw;

    float depthHere = projCoords.z;
    float depthScreen = shadowAtlas.Sample(samplerTable[pc.shadowAtlasSampler_ID], uv).x;

    //float bias = max(SHADOW_BIAS_MAX * (1.0 - NdotL), SHADOW_BIAS_MIN);
    return depthHere > depthScreen ? 1.0 : 0.0;
}

float4 main(PSInput input) : SV_Target
{
	float2 uv = frac(input.texCoord);
	
	SamplerState textureSampler = samplerTable[pc.textureSampler_ID];
	SamplerState cubemapSampler = samplerTable[pc.cubemapSampler_ID];
	
	FrameConstants frameData	= frameConstantsTable[pc.frameDataBuffer_ID][0];
	MaterialData materialData	= materialDataTable[pc.materialDataBuffer_ID][pc.material_ID];
	
	float3 albedo					= texture2DTable[materialData.diffuseTexture_ID]	.Sample(textureSampler, uv).rgb;
	float  ambientOcclusion			= texture2DTable[materialData.aoTexture_ID]			.Sample(textureSampler, uv).r;
	float3 ambientRoughnessMetallic	= texture2DTable[materialData.armTexture_ID]		.Sample(textureSampler, uv).rgb;
	float3 normal					= texture2DTable[materialData.normalTexture_ID]		.Sample(textureSampler, uv).rgb;
	float3 emissive					= texture2DTable[materialData.emissiveTexture_ID]	.Sample(textureSampler, uv).rgb;
	
	// light testing white surface
//	albedo					    = 1.0;
//	ambientOcclusion			= texture2DTable[materialData.aoTexture_ID].Sample(textureSampler, uv).r;
//	ambientRoughnessMetallic	= float3(texture2DTable[materialData.armTexture_ID].Sample(textureSampler, uv).r, 1.0, 0.0);
//	normal					    = float3(0.5, 0.5, 1.0);
//	emissive					= 0.0;
	
	ambientOcclusion		+= ambientRoughnessMetallic.r;
	float roughnessValue	 = ambientRoughnessMetallic.g;
	float metallicValue		 = ambientRoughnessMetallic.b;
	
	float3 F0 = lerp(0.04, albedo, metallicValue);

	normal = normalize(mul(input.tbn, 2.0*normal - 1.0));

	float3 viewDir = normalize(frameData.viewPos.xyz - input.position.xyz);
	
	float NdotV = max(0.0, dot(normal, viewDir));
	
	float3 Lo = 0.0;
	
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
	    PointLight light = lightsTable[pc.lightBuffer_ID][i];

		float3 deltaX = light.position.xyz - input.position.xyz;
		float distanceSquared = dot(deltaX, deltaX);

		float attenuation = 1.0 / (0.01 * distanceSquared);
		float3 radiance = light.colour.rgb * attenuation;
		
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

		float shadow = calculateShadow(mul(light.lightSpaceMatrix, input.position), NdotL, normal, light.atlasRegion);
		
        Lo += radiance * NdotL * (kD * diffuse + specular) * (1.0 - shadow);
    }

	float3 F = fresnelSchlick(NdotV, F0, roughnessValue);
	float3 kD = (1.0 - F) * (1.0 - metallicValue);
	
	float3 reflected = reflect(-viewDir, normal);
	
	float prefilterMipLevel = roughnessValue * MAX_REFLECTION_LOD;
	float3 prefilteredColour = textureCubeTable[pc.prefilterMap_ID].SampleLevel(cubemapSampler, reflected, prefilterMipLevel).rgb;
	
	float2 environmentBRDF = texture2DTable[pc.brdfLUT_ID].Sample(textureSampler, float2(NdotV, roughnessValue)).xy;
	
	float3 specular = prefilteredColour * (F * environmentBRDF.x + environmentBRDF.y);
	
	float3 irradiance = textureCubeTable[pc.irradianceMap_ID].Sample(cubemapSampler, normal).rgb;
	float3 diffuse = irradiance * albedo;
	float3 ambient = (kD * diffuse + specular) * ambientOcclusion;
	
	float3 finalColour = ambient + Lo + emissive;
	
	finalColour *= input.colour;
	
    return float4(finalColour, 1.0);
}

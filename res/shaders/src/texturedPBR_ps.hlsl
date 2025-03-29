#pragma shader_model 5_1

#include "bindless.hlsl"

#include "model_common.inc"

#define MAX_REFLECTION_LOD 4.0

struct PSInput
{
	[[vk::location(VS_MODEL_OUT_SLOT_COLOR)]]
    float3 colour : COLOR;
    
	[[vk::location(VS_MODEL_OUT_SLOT_POSITION)]]
    float3 position : TEXCOORD0;
    
	[[vk::location(VS_MODEL_OUT_SLOT_UV)]]
    float2 texCoord : TEXCOORD1;
    
	[[vk::location(VS_MODEL_OUT_SLOT_TBN)]]
    float3x3 tbn : TEXCOORD3;
};

float3 fresnelSchlick(float cosT, float3 F0, float roughness)
{
	float t = pow(1.0 - max(cosT, 0.0), 5.0);
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

float4 main(PSInput input) : SV_Target
{
	float2 uv = frac(input.texCoord);
	
	SamplerState textureSampler = samplerTable[pc.textureSampler_ID];
	SamplerState cubemapSampler = samplerTable[pc.cubemapSampler_ID];
	
	FrameConstants frameData	= bufferTable[pc.frameDataBuffer_ID]		.Load<FrameConstants>(0);
	MaterialData materialDats	= bufferTable[pc.materialDataBuffer_ID]		.Load<MaterialData>(pc.material_ID);
	
	float3 albedo				= texture2DTable[materialData.diffuseTexture_ID]	.Sample(textureSampler, uv).rgb;
	float  ambientOcclusion		= texture2DTable[materialData.aoTexture_ID]			.Sample(textureSampler, uv).r;
	float3 metallicRoughness	= texture2DTable[materialData.mrTexture_ID]			.Sample(textureSampler, uv).rgb;
	float3 normal				= texture2DTable[materialData.normalTexture_ID]		.Sample(textureSampler, uv).rgb;
	float3 emissive				= texture2DTable[materialData.emissiveTexture_ID]	.Sample(textureSampler, uv).rgb;
	
	ambientOcclusion		+= metallicRoughness.r;
	float roughnessValue	 = metallicRoughness.g;
	float metallicValue		 = metallicRoughness.b;
	
	float3 F0 = lerp(0.04, albedo, metallicValue);
	
	normal = normalize(mul(normalize(2.0 * normal - 1.0), input.tbn));
	float3 viewDir = normalize(frameData.viewPos.xyz - input.position);
	
	float NdotV = max(0.0, dot(normal, viewDir));
	
	float3 Lo = 0.0;
	
	/*
	for (int i = 0; i < MAX_N_LIGHTS; i++)
	{
		if (dot(frameData.lights[i].colour, frameData.lights[i].colour) <= 0.01)
			continue;
		
		float3 deltaX = frameData.lights[i].position - input.position;
		float distanceSquared = dot(deltaX, deltaX);
		float attenuation = 1.0 / (0.125 * distanceSquared);
		float3 radiance = frameData.lights[i].colour * attenuation;
		
		float3 lightDir = normalize(deltaX);
		float3 halfwayDir = normalize(lightDir + viewDir);
	
		float3 reflected = reflect(-viewDir, normal);
		reflected = normalize(reflected);
	
		float NdotL = max(0.0, dot(normal, lightDir));
		float NdotH = max(0.0, dot(normal, halfwayDir));
		
		float cosT = max(0.0, dot(halfwayDir, viewDir));
		
		float3 F = fresnelSchlick(cosT, F0, 0.0);
		
		float NDF = distributionGGX(NdotH, roughnessValue);
		float G = geometrySmith(NdotV, NdotL, roughnessValue);
		
		float3 kD = (1.0 - F) * (1.0 - metallicValue);
		
		float3 diffuse = albedo / MATH_PI;
		float3 specular = (F * G * NDF) / (4.0 * NdotL * NdotV + 0.0001);
		
		Lo += (kD * diffuse + specular) * radiance * NdotL;
	}
	*/
	
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

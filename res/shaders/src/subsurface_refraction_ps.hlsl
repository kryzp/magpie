#include "common_ps.hlsl"

struct MaterialUBO
{
	float eta;
	float depth;
	float extinction;
	float _padding1;
};

ConstantBuffer<MaterialUBO> materialData : register(b2);

struct PSInput
{
	[[vk::location(0)]] float3 colour : COLOR;
	[[vk::location(1)]] float3 position : TEXCOORD0;
	[[vk::location(2)]] float2 texCoord : TEXCOORD1;
	[[vk::location(3)]] float3 fragPos : TEXCOORD2;
	[[vk::location(4)]] float3x3 tbn : TEXCOORD3;
};

#define MAX_REFLECTION_LOD 4.0

TextureCube localIrradianceMap : register(t3);
SamplerState localIrradianceMapSampler : register(s3);

TextureCube localPrefilterMap : register(t4);
SamplerState localPrefilterMapSampler : register(s4);

Texture2D brdfLUT : register(t5);
SamplerState brdfLUTSampler : register(s5);

Texture2D diffuseTexture : register(t6);
Texture2D aoTexture : register(t7);
Texture2D mrTexture : register(t8);
Texture2D normalTexture : register(t9);
Texture2D emissiveTexture : register(t10);

SamplerState diffuseSampler : register(s6);
SamplerState aoSampler : register(s7);
SamplerState mrSampler : register(s8);
SamplerState normalSampler : register(s9);
SamplerState emissiveSampler : register(s10);

float3 fresnelSchlick(float NdotV, float3 F0)
{
	float t = pow(1.0 - max(NdotV, 0.0), 5.0);
	float3 F90 = 1.0;
	return lerp(F0, F90, t);
}

float2 surfaceParallax(float2 uv, float3 refracted)
{
	float2 p = materialData.depth / refracted.z * refracted.xy;
	return uv - p;
}

float4 main(PSInput input) : SV_Target
{
	float2 uv = frac(input.texCoord);
	
	float3 surfaceNormal = input.tbn[2];
	
	float3 viewDir = normalize(frameData.viewPos.xyz - input.fragPos.xyz);
	float3 viewDirTangent = normalize(mul(viewDir, transpose(input.tbn)));
	
	float3 reflectedViewDir = reflect(-viewDir, surfaceNormal);
	float3 refractedViewDirTangent = refract(-viewDirTangent, float3(0.0, 0.0, 1.0), materialData.eta);
	
	float2 refractedUV = surfaceParallax(uv, refractedViewDirTangent);
	
	float3 normal = normalTexture.Sample(normalSampler, refractedUV).rgb;
	normal = normalize(mul(normalize(2.0 * normal - 1.0), input.tbn));
	
	float3 albedo = diffuseTexture.Sample(diffuseSampler, refractedUV).rgb;
	float3 diffuse = albedo * localIrradianceMap.Sample(localIrradianceMapSampler, normal).rgb;
	
	float3 specular = localPrefilterMap.Sample(localIrradianceMapSampler, reflectedViewDir).rgb;
	
	float d = materialData.depth * sqrt(1.0 + 1.0/(refractedViewDirTangent.z*refractedViewDirTangent.z));
	d = min(d, 9999.9);
	
	float transmittance = exp(-materialData.extinction * d);
	
	float3 F = fresnelSchlick(viewDirTangent.z, 0.04);
	
	//diffuse = lerp(float3(0.23, 0.19, 0.1), diffuse, transmittance);
	
	float3 col = lerp(diffuse, specular, F);
	
	return float4(col, 1.0);
}

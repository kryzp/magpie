#include "bindless.hlsl"

#include "model_common.hlsl"

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
	
	SamplerState textureSampler = samplerTable[pc.textureSampler_ID];
	SamplerState cubemapSampler = samplerTable[pc.cubemapSampler_ID];
	
	float3 surfaceNormal = input.tbn[2];
	
	float3 viewDir = normalize(frameData.viewPos.xyz - input.position.xyz);
	float3 viewDirTangent = normalize(mul(viewDir, transpose(input.tbn)));
	
	float3 reflectedViewDir = reflect(-viewDir, surfaceNormal);
	float3 refractedViewDirTangent = refract(-viewDirTangent, float3(0.0, 0.0, 1.0), materialData.eta);
	
	float2 refractedUV = surfaceParallax(uv, refractedViewDirTangent);
	
	float3 textureNormal = texture2DTable[pc.normalTexture_ID].Sample(textureSampler, refractedUV).rgb;
	textureNormal = normalize(mul(normalize(2.0 * textureNormal - 1.0), input.tbn));
	
	float3 albedo = texture2DTable[pc.diffuseTexture_ID].Sample(textureSampler, refractedUV).rgb;
	float3 diffuse = albedo * textureCubeTable[pc.irradianceMap_ID].Sample(cubemapSampler, textureNormal).rgb;
	
	float3 specular = textureCubeTable[pc.prefilterMap_ID].Sample(cubemapSampler, reflectedViewDir).rgb;
	
	if (refractedUV.x < 0.0 || refractedUV.y < 0.0 || refractedUV.x > 1.0 || refractedUV.y > 1.0) {
		diffuse = textureCubeTable[pc.prefilterMap_ID].Sample(cubemapSampler, -viewDir).rgb;
	}
	
	float3 F = fresnelSchlick(viewDirTangent.z, 0.04);
	
	float3 col = lerp(diffuse, specular, F);
	
	return float4(col, 1.0);
}

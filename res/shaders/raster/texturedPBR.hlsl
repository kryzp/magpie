#include "common_ps.hlsl"

struct PSInput
{
    [[vk::location(VS_OUT_SLOT_POSITION)]] float3 position : TEXCOORD0;
    [[vk::location(VS_OUT_SLOT_UV)]] float2 texCoord : TEXCOORD1;
    [[vk::location(VS_OUT_SLOT_COLOUR)]] float3 colour : COLOR;
    [[vk::location(VS_OUT_SLOT_TANGENT_FRAG_POS)]] float3 fragPos : TEXCOORD2;
    [[vk::location(VS_OUT_SLOT_TBN_MATRIX)]] float3x3 tbn : TEXCOORD4;
};

Texture2D diffuseTexture : register(t3);
Texture2D amrTexture : register(t4);
Texture2D normalTexture : register(t5);
Texture2D emissiveTexture : register(t6);

SamplerState diffuseSampler : register(s3);
SamplerState amrSampler : register(s4);
SamplerState normalSampler : register(s5);
SamplerState emissiveSampler : register(s6);

float3 fresnelSchlick(float cosT, float3 F0)
{
	return lerp(F0, 1.0, pow(1.0 - max(cosT, 0.0), 5.0));
}

float distributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	
	float NdotH = max(dot(N, H), 0.0);
	
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;

	return a2 / (denom * denom * MATH_PI);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;
	
	return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	
	float ggx2 = geometrySchlickGGX(NdotV, roughness);
	float ggx1 = geometrySchlickGGX(NdotL, roughness);
	
	return ggx1 * ggx2;
}

float4 main(PSInput input) : SV_Target
{
	float2 uv = frac(input.texCoord);
	
	float3 albedo = diffuseTexture.Sample(diffuseSampler, uv).rgb;
	float3 ambientMetallicRoughness = amrTexture.Sample(amrSampler, uv).rgb;
	float3 normal = normalTexture.Sample(normalSampler, uv).rgb;
	float3 emissive = emissiveTexture.Sample(emissiveSampler, uv).rgb;
	
	float ambientOcclusion = ambientMetallicRoughness.r;
	float roughnessValue = ambientMetallicRoughness.b;
	float metallicValue = ambientMetallicRoughness.g;
    
	normal = normalize(mul(input.tbn, normalize(2.0 * normal - 1.0)));

	float3 viewDir = normalize(frameData.viewPos.xyz - input.fragPos);
	float3 lightDir = -normalize(float3(1.0, -1.0, -1.0));
    
	float3 halfwayDir = normalize(lightDir + viewDir);
	
	float3 F0 = lerp(0.04, albedo, metallicValue);
	float3 F = fresnelSchlick(dot(halfwayDir, viewDir), F0);
	float NDF = distributionGGX(normal, halfwayDir, roughnessValue);
	float G = geometrySmith(normal, viewDir, lightDir, roughnessValue);
	
	float shadow = 1.0;
	
	float3 kD = (1.0 - F) * (1.0 - metallicValue);
	
	float3 ambient = 0.3 * ambientOcclusion * albedo;
	float3 diffuse = kD * albedo;
	float3 specular = (F * NDF * G) / (4.0*dot(normal, lightDir)*dot(normal, viewDir) + 0.0001);

	float3 finalColour = ambient + emissive + shadow * (diffuse + specular);
	
    finalColour *= input.colour;
	return float4(finalColour, 1.0);
}

/*
    for (int i = 0; i < MAX_N_LIGHTS; i++)
    {
        if (dot(frameData.lights[i].colour, frameData.lights[i].colour) <= 0.01)
            continue;

        float type = frameData.lights[i].type;
        float3 lightDir = float3(0.0, 0.0, 0.0);

        // Sun
        if (type == 0.0)
        {
            lightDir = -normalize(mul(input.tbn, frameData.lights[i].direction));
        }

        // Spotlight
        else if (type == 1.0)
        {
            // todo
        }

        // Point light
        else if (type == 2.0)
        {
            float3 tangentLightPos = mul(frameData.lights[i].position, input.tbn);
            lightDir = normalize(tangentLightPos - input.tangentFragPos);
        }

        float3 halfwayDir = normalize(lightDir + viewDir);
        
		float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, float3(metallicValue, metallicValue, metallicValue));
		float3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);
        
		float NDF = distributionGGX(normal, halfwayDir, roughnessValue);
		float G = geometrySmith(normal, viewDir, lightDir, roughnessValue);
        
		float3 kD = (1.0 - F) * (1.0 - metallicValue);
		diffuse += kD * albedo * frameData.lights[i].colour;

		float cosThetaLight = max(dot(normal, lightDir), 0.0);
		float cosThetaView = max(dot(normal, viewDir), 0.0);
		specular += (NDF * G * F) / (4.0 * cosThetaView * cosThetaLight + 0.001);
	}

*/

#include "common_ps.hlsl"

struct PSInput
{
    [[vk::location(VS_OUT_SLOT_POSITION)]] float3 position : TEXCOORD0;
    [[vk::location(VS_OUT_SLOT_UV)]] float2 texCoord : TEXCOORD1;
    [[vk::location(VS_OUT_SLOT_COLOUR)]] float3 colour : COLOR;
    [[vk::location(VS_OUT_SLOT_TANGENT_FRAG_POS)]] float3 tangentFragPos : TEXCOORD2;
    [[vk::location(VS_OUT_SLOT_TANGENT_VIEW_POS)]] float3 tangentViewPos : TEXCOORD3;
    [[vk::location(VS_OUT_SLOT_TBN_MATRIX)]] float3x3 tbn : TEXCOORD4;
};

Texture2D diffuseTexture : register(t3);
Texture2D specularTexture : register(t4);
Texture2D normalTexture : register(t5);

SamplerState diffuseSampler : register(s3);
SamplerState specularSampler : register(s4);
SamplerState normalSampler : register(s5);

float4 main(PSInput input) : SV_Target
{
	float3 diffuseColour = diffuseTexture.Sample(diffuseSampler, input.texCoord).rgb;
	float3 specularColour = specularTexture.Sample(specularSampler, input.texCoord).rgb;
	float3 normal = normalTexture.Sample(normalSampler, input.texCoord).rgb;
    
    normal = normalize(2.0*normal - 1.0);

    float3 viewDir = normalize(input.tangentViewPos - input.tangentFragPos);
    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);

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

        diffuse += max(dot(lightDir, normal), 0.0) * diffuseColour * frameData.lights[i].colour;
        specular += pow(max(dot(halfwayDir, normal), 0.0), 32.0) * specularColour;
    }

    float shadow = 1.0;
    float3 ambient = 0.15 * diffuseColour;
    float3 finalColour = ambient + shadow * (diffuse + specular);

    finalColour *= input.colour;
    return float4(finalColour, 1.0);
}

#include "common.hlsl"

struct PSInput
{
	[[vk::location(0)]]
	float3 worldPos : TEXCOORD0;
};

TextureCube environmentMap : register(t0);
SamplerState samplerState : register(s0);

float4 main(PSInput input) : SV_Target
{
	float3 normal = normalize(input.worldPos);
	float3 right = normalize(cross(float3(0.0, 1.0, 0.0), normal));
	float3 up = normalize(cross(normal, right));
	
	float dt = 0.05;
	float nSamples = 0.0;
	
	float3 irradiance = 0.0;
	
	for (float phi = 0.0; phi < 2.0 * MATH_PI; phi += dt)
	{
		for (float theta = 0.0; theta < 0.5 * MATH_PI; theta += dt)
		{
			float3 tangentSample = float3(
				sin(theta) * cos(phi),
				sin(theta) * sin(phi),
				cos(theta)
			);
			
			float3x3 tangentToWorld = transpose(float3x3(right, up, normal));
			
			float3 sampleVec = mul(tangentToWorld, tangentSample);

			irradiance += environmentMap.Sample(samplerState, sampleVec).rgb * cos(theta) * sin(theta);
			
			nSamples++;
		}
	}
	
	irradiance *= MATH_PI / nSamples;

	return float4(irradiance, 1.0);
}

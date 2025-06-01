#include "primitive_vs.slang"

[[vk::binding(0)]] SamplerCube environmentMap;

const static float PI = 3.14159265;

[shader("fragment")]
float4 fragmentMain(VS_Output input)
{
	float3 normal = normalize(input.worldPosition);
	float3 right = normalize(cross(float3(0.0, 1.0, 0.0), normal));
	float3 up = normalize(cross(normal, right));

	float dt = 0.05;
	float n_samples = 0.0;

	float3 irradiance = 0.0;
	
	for (float phi = 0.0; phi < 2.0 * PI; phi += dt)
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += dt)
		{
			float3 sample = float3(
				sin(theta) * cos(phi),
				sin(theta) * sin(phi),
				cos(theta)
			);
			
			float3x3 tangentToWorld = transpose(float3x3(right, up, normal));
			
			float3 sampleVec = mul(tangentToWorld, sample);

			irradiance += environmentMap.Sample(sampleVec).rgb * cos(theta) * sin(theta);
			
			n_samples++;
		}
	}

	irradiance *= PI / n_samples;

	return float4(irradiance, 1.0);
}

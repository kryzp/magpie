#include "primitive_vs.slang"

const static float PI = 3.14159265;
const static int SAMPLE_COUNT = 1024;

struct Data
{
	float roughness;
	float _padding0;
	float _padding1;
	float _padding2;
};

[[vk::binding(0)]] ConstantBuffer<Data> ubo;
[[vk::binding(1)]] SamplerCube environmentMap;

float radicalInverse_VdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

float2 hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), radicalInverse_VdC(i));
}

float3 importanceSampleGGX(float2 Xi, float3 normal, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
	float3 H = float3(
		cos(phi) * sinTheta,
		sin(phi) * sinTheta,
		cosTheta
	);
	
	float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, normal));
	float3 bitangent = cross(normal, tangent);
	
	float3x3 inverseTBN = transpose(float3x3(tangent, bitangent, normal));
	
	float3 sampleVec = mul(inverseTBN, H);
	
	return normalize(sampleVec);
}

float distributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;

	return a2 / (denom * denom * PI);
}

[shader("fragment")]
float4 fragmentMain(VS_Output input) : SV_Target
{
    uint envWidth, envHeight;
    environmentMap.GetDimensions(envWidth, envHeight);
	
	float3 normal = normalize(input.worldPosition);
	float3 R = normal;
	float3 V = R;
	
	float totalWeight = 0.0;
	
	float3 result = 0.0;
	
	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		float2 Xi = hammersley(i, SAMPLE_COUNT);
		float3 H = importanceSampleGGX(Xi, normal, ubo.roughness);
		float3 L = normalize(2.0*dot(V, H)*H - V);
		
		float NdotL = max(0.0, dot(normal, L));
		
		if (NdotL > 0.0)
		{
			float NdotH = max(0.0, dot(normal, H));
			float HdotV = max(0.0, dot(H, V));
			
			float D = distributionGGX(NdotH, ubo.roughness);
			
			float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;
			
			float saTexel = 4.0 * PI / (6.0 * float(envWidth) * float(envHeight));
			float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

			float mipLevel = ubo.roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            result += min(10.0, environmentMap.SampleLevel(L, mipLevel).rgb) * NdotL;
			totalWeight += NdotL;
		}
	}
	
	result /= totalWeight;
	
	return float4(result, 1.0);
}

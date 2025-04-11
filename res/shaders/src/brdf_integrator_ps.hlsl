struct PSInput
{
	[[vk::location(0)]]
	float2 texCoord : TEXCOORD0;
};

#define MATH_PI 3.14159265359
#define SAMPLE_COUNT 1024

float radicalInverse_VdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), radicalInverse_VdC(i));
}

float3 importanceSampleGGX(float2 Xi, float3 normal, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * MATH_PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
    // from spherical coordinates to cartesian coordinates
	float3 H = float3(
		cos(phi) * sinTheta,
		sin(phi) * sinTheta,
		cosTheta
	);
	
    // from tangent-space vector to world-space sample vector
	float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, normal));
	float3 bitangent = cross(normal, tangent);
	
	float3x3 inverseTBN = transpose(float3x3(tangent, bitangent, normal));
	
	float3 sampleVec = mul(inverseTBN, H);
	
	return normalize(sampleVec);
}

float geometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness;
	float k = r * r / 2.0;
	
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
	float NdotV = input.texCoord.x;
	float roughness = input.texCoord.y;
	
	float3 V = float3(
		sqrt(1.0 - NdotV * NdotV),
		0.0,
		NdotV
	);

	float A = 0.0;
	float B = 0.0;

	float3 N = float3(0.0, 0.0, 1.0);
    
	for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
		float2 Xi = hammersley(i, SAMPLE_COUNT);
		float3 H = importanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);
		
		float NdotL = max(0.0, dot(N, L));
		
		if (NdotL > 0.0)
        {
			float NdotH = max(0.0, H.z);
			float VdotH = max(0.0, dot(V, H));
			
			float G = geometrySmith(NdotV, NdotL, roughness);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5.0);

			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}
	
	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);
	
	return float4(A, B, 0.0, 1.0);
}

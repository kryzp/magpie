
const static float PI = 3.14159265;

struct FrameData
{
	float4x4 view;
	float4x4 projection;
	float4x4 view_projection;
	float4x4 view_projection_no_translation;
	float4x4 inv_view;
	float4x4 inv_projection;
	float4 camera_position;
	float4 window_resolution;
	float time;
	float _padding[3];
};

struct TransformData
{
	float4x4 model_matrix;
	float4x4 normal_matrix;
};

// NOTE(kp): We use a right-handed Z-up world
//           coordinate system, but cubemap
//           sampling requires Y-up left handed.
float3 WorldToCubemap(float3 v)
{
	return float3(v.x, v.z, v.y);
}

float RadicalInverse_VdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 FresnelSchlick(float VdotH, float3 F0, float roughness)
{
	float t = pow(1.0 - max(VdotH, 0.0), 5.0);
	float3 F90 = max(1. - roughness, F0);
	return lerp(F0, F90, t);
}

float DistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	
	float denom = NdotH * NdotH * (a2 - 1.) + 1.;
	
	return a2 / max(0.0001, denom * denom * PI);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.;
	float k = r * r / 8.;
	
	return NdotV / lerp(k, 1., NdotV);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	
	return ggx1 * ggx2;
}

float3 ImportanceSampleGGX(float2 Xi, float3 normal, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2. * PI * Xi.x;
	
	float cos_theta = sqrt((1. - Xi.y) / (1. + (a * a - 1.) * Xi.y));
	float sin_theta = sqrt(1. - cos_theta * cos_theta);
	
	float3 H = float3(
		cos(phi) * sin_theta,
		sin(phi) * sin_theta,
		cos_theta
	);
	
	float3 up = abs(normal.z) < 0.999 ? float3(0., 0., 1.) : float3(1., 0., 0.);
	float3 tangent = normalize(cross(up, normal));
	float3 bitangent = cross(normal, tangent);
	
	float3x3 inverse_tbn = transpose(float3x3(tangent, bitangent, normal));
	float3 sample_vector = mul(inverse_tbn, H);
	
	return normalize(sample_vector);
}

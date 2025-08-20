
const static float PI = 3.14159265;

struct FrameData
{
	float4x4 projection_matrix;
	float4x4 view_matrix;
	float4 camera_position;
	float4 window_resolution;
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

float3 FresnelSchlick(float VdotH, float3 F0, float roughness)
{
	float t = pow(1. - max(VdotH, 0.), 5.);
	float3 F90 = max(1. - roughness, F0);
	return lerp(F0, F90, t);
}

float DistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	
	float denom = NdotH * NdotH * (a2 - 1.) + 1.;
	
	return a2 / max(0.00001, denom * denom * PI);
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

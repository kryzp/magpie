module math;

namespace math
{
	public static const float PI = 3.14159265;

	// We use a right-handed Z-up world coordinate system, but cubemap
	// sampling requires Y-up left handed.
	// Thanks RenderMan!!
	// Warning: Does NOT normalize.
	public float3 WorldToCubemap(float3 v)
	{
		return float3(v.x, v.z, v.y);
	}

	public void BuildOrthonormalBasis(float3 normal, out float3 tangent, out float3 bitangent)
	{
		float3 u = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);

		tangent = normalize(cross(normal, u));
		bitangent = normalize(cross(tangent, normal));
	}
}

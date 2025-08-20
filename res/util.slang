const static float PI = 3.14159265;

// NOTE(kp): We use a right-handed Z-up world
//           coordinate system, but cubemap
//           sampling requires Y-up left handed.
float3 WorldToCubemap(float3 v)
{
	return float3(v.x, v.z, v.y);
}

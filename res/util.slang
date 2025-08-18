const static float PI = 3.14159265;

float3 WorldToCubemap(float3 v)
{
	return float3(v.x, v.y, -v.z);
}

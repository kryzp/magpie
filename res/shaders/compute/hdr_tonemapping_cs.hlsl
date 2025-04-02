
struct PushConstants
{
	uint width;
	uint height;
	float exposure;
	float blurIntensity;
};

[[vk::push_constant]]
PushConstants pc;

RWTexture2D<float4> target : register(u0);
Texture2D<float4> bloom : register(t1);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThread : SV_DispatchThreadID)
{
	if (dispatchThread.x < 0 ||
		dispatchThread.x >= pc.width ||
		dispatchThread.y < 0 ||
		dispatchThread.y >= pc.height)
	{
		return;
	}

	float3 colour = target[dispatchThread.xy].rgb;
	
	float bw = dot(colour, float3(0.299, 0.587, 0.114)) * (pc.exposure - 1.0);
	
	target[dispatchThread.xy] = float4(bw, bw, bw, 1.0);
}

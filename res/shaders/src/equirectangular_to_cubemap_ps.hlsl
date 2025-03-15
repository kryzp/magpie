struct PSInput
{
	[[vk::location(0)]]
	float3 worldPos : TEXCOORD0;
};

Texture2D texture : register(t0);
SamplerState samplerState : register(s0);

float2 sampleSphericalMap(float3 v)
{
	float2 uv = float2(atan2(v.z, v.x), asin(v.y));
	uv *= float2(0.1591, 0.3183);
	uv += 0.5;
	uv = 1.0 - uv;
	return uv;
}

float4 main(PSInput input) : SV_TARGET
{
	float2 uv = sampleSphericalMap(normalize(input.worldPos));
	float3 col = texture.Sample(samplerState, uv).rgb;
	
	return float4(col, 1.0);
}

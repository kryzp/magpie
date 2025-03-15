struct PushConstants
{
	int mipLevel;
};

[[vk::push_constant]]
PushConstants pc;

struct PSInput
{
	[[vk::location(0)]]
	float2 texCoord : TEXCOORD0;
};

Texture2D renderTarget : register(t0);
SamplerState samplerState : register(s0);

float rgbToLuminance(float3 col)
{
	return dot(col, float3(0.2126, 0.7152, 0.0722));
}

float karisAverage(float3 col)
{
    // Formula is 1 / (1 + luma)
	float luma = rgbToLuminance(pow(col, 1.0 / 2.2)) * 0.25;
	
	return 1.0 / (1.0 + luma);
}

float4 main(PSInput input) : SV_TARGET
{
	uint width, height;
	renderTarget.GetDimensions(width, height);
	
	float2 texelSize = 1.0 / float2(width, height);

	float dx = texelSize.x;
	float dy = texelSize.y;
	
	float2 uv = input.texCoord;
	
	float3 a = renderTarget.Sample(samplerState, float2(uv.x - 2*dx,	uv.y + 2*dy	)).rgb;
	float3 b = renderTarget.Sample(samplerState, float2(uv.x,			uv.y + 2*dy	)).rgb;
	float3 c = renderTarget.Sample(samplerState, float2(uv.x + 2*dx,	uv.y + 2*dy	)).rgb;
	float3 d = renderTarget.Sample(samplerState, float2(uv.x - 2*dx,	uv.y		)).rgb;
	float3 e = renderTarget.Sample(samplerState, float2(uv.x,			uv.y		)).rgb;
	float3 f = renderTarget.Sample(samplerState, float2(uv.x + 2*dx,	uv.y		)).rgb;
	float3 g = renderTarget.Sample(samplerState, float2(uv.x - 2*dx,	uv.y - 2*dy	)).rgb;
	float3 h = renderTarget.Sample(samplerState, float2(uv.x,			uv.y - 2*dy	)).rgb;
	float3 i = renderTarget.Sample(samplerState, float2(uv.x + 2*dx,	uv.y - 2*dy	)).rgb;
	float3 j = renderTarget.Sample(samplerState, float2(uv.x -   dx,	uv.y +   dy	)).rgb;
	float3 k = renderTarget.Sample(samplerState, float2(uv.x +   dx,	uv.y +   dy	)).rgb;
	float3 l = renderTarget.Sample(samplerState, float2(uv.x -   dx,	uv.y -   dy	)).rgb;
	float3 m = renderTarget.Sample(samplerState, float2(uv.x +   dx,	uv.y -   dy	)).rgb;
	
	float3 downsample = 0.0;
	
	if (pc.mipLevel == 0)
	{
		float3 group0 = (a + b + d + e) * (0.125 / 4.0);
		float3 group1 = (b + c + e + f) * (0.125 / 4.0);
		float3 group2 = (d + e + g + h) * (0.125 / 4.0);
		float3 group3 = (e + f + h + i) * (0.125 / 4.0);
		float3 group4 = (j + k + l + m) * (0.500 / 4.0);
		
		group0 *= karisAverage(group0);
		group1 *= karisAverage(group1);
		group2 *= karisAverage(group2);
		group3 *= karisAverage(group3);
		group4 *= karisAverage(group4);
		
		downsample = group0 + group1 + group2 + group3 + group4;
	}
	else
	{
		downsample = e * 0.125;
		downsample += (a + c + g + i) * 0.03125;
		downsample += (b + d + f + h) * 0.0625;
		downsample += (j + k + l + m) * 0.125;
	}
	
	downsample = max(downsample, 0.0001);
	
	return float4(downsample, 1.0);
}

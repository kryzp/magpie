
struct VSInput
{
	[[vk::location(0)]]
	float3 position : POSITION;
	
	[[vk::location(1)]]
	float2 uv : TEXCOORD0;
};

struct VSOutput
{
	float4 svPosition : SV_Position;
	
	[[vk::location(0)]]
	float2 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output;
	output.svPosition = float4(input.position, 1.0);
	output.texCoord = input.uv;

	return output;
}

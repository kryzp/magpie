#include "common_fxc.inc"

struct VSInput
{
	[[vk::location(VS_ATT_SLOT_POSITION)]]
	float3 position : POSITION;
	
	[[vk::location(VS_ATT_SLOT_UV)]]
	float2 uv : TEXCOORD0;
	
	[[vk::location(VS_ATT_SLOT_COLOUR)]]
	float3 colour : COLOR;
	
	[[vk::location(VS_ATT_SLOT_NORMAL)]]
	float3 normal : NORMAL;
	
	[[vk::location(VS_ATT_SLOT_TANGENT)]]
	float3 tangent : TANGENT;
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
	output.texCoord = input.uv;
	output.svPosition = float4(input.position, 1.0);

	return output;
}

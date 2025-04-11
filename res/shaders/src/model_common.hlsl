#ifndef MODEL_COMMON_INC_
#define MODEL_COMMON_INC_

#define VS_MODEL_ATT_SLOT_POSITION 0
#define VS_MODEL_ATT_SLOT_UV 1
#define VS_MODEL_ATT_SLOT_COLOUR 2
#define VS_MODEL_ATT_SLOT_NORMAL 3
#define VS_MODEL_ATT_SLOT_TANGENT 4
#define VS_MODEL_ATT_SLOT_BITANGENT 5

#define VS_MODEL_OUT_SLOT_COLOR 0
#define VS_MODEL_OUT_SLOT_POSITION 1
#define VS_MODEL_OUT_SLOT_UV 2
#define VS_MODEL_OUT_SLOT_TBN 3

struct VSInput
{
	[[vk::location(VS_MODEL_ATT_SLOT_POSITION)]]
	float3 position : POSITION;

	[[vk::location(VS_MODEL_ATT_SLOT_UV)]]
	float2 uv : TEXCOORD0;

	[[vk::location(VS_MODEL_ATT_SLOT_COLOUR)]]
	float3 colour : COLOR;

	[[vk::location(VS_MODEL_ATT_SLOT_NORMAL)]]
	float3 normal : NORMAL;

	[[vk::location(VS_MODEL_ATT_SLOT_TANGENT)]]
	float3 tangent : TANGENT;

	[[vk::location(VS_MODEL_ATT_SLOT_BITANGENT)]]
	float3 bitangent : BITANGENT;
};

#endif // MODEL_COMMON_INC_

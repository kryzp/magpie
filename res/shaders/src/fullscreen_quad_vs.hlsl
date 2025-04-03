
struct VSOutput
{
	float4 svPosition : SV_Position;
	
	[[vk::location(0)]]
	float2 texCoord : TEXCOORD0;
};

VSOutput main(uint id : SV_VertexID)
{
    VSOutput output;
	
    output.texCoord = float2((id << 1) & 2, id & 2);
    output.svPosition = float4(output.texCoord.x*2.0 - 1.0, 1.0 - 2.0*output.texCoord.y, 0.0, 1.0);

    return output;
}

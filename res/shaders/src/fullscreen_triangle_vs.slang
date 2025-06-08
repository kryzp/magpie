struct VS_Output
{
    float4 sv_position : SV_Position;
    [[vk::location(0)]] float2 uv;
};

[shader("vertex")]
VS_Output vertexMain(uint id : SV_VertexID)
{
    VS_Output output;
    output.uv = float2((id << 1) & 2, id & 2);
    output.sv_position = float4(output.uv.x*2.0 - 1.0, 1.0 - 2.0*output.uv.y, 0.0, 1.0);

    return output;
}

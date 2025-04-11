float main(float4 svPosition : SV_Position) : SV_Depth
{
    return svPosition.z;
}

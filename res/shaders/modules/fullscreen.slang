module fullscreen;

namespace fullscreen
{
	public struct VS_Output
	{
		public float4 sv_position : SV_Position;
		[vk::location(0)] public float2 uv;
	};

	// standard 3-vertex fullscreen triangle.
	public VS_Output FullscreenVS(uint id)
	{
		VS_Output output;
		output.uv = float2(id & 2, (id << 1) & 2);
		output.sv_position = float4(output.uv.x * 2.0 - 1.0, 1.0 - 2.0 * output.uv.y, 0.0, 1.0);

		return output;
	}
}

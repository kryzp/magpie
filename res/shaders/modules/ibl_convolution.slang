module ibl_convolution;

namespace ibl_convolution
{
	public struct VS_Output
	{
		public float4 sv_position : SV_Position;
		[vk::location(0)] public float3 position;
	};

	// render standard unit cube via SV_ViewID transform matrices
	public VS_Output CubemapFaceVS(uint vertex_id,
	                               uint view_id,
	                               float4x4 *transform_matrices,
	                               float3 *vertices)
	{
		VS_Output output;
		output.position = vertices[vertex_id];
		output.sv_position = mul(transform_matrices[view_id], float4(output.position, 1.0));
		return output;
	}
}

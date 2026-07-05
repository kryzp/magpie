module frame;

namespace frame
{
	public struct FrameData
	{
		public float4x4 view;
		public float4x4 projection;
		public float4x4 view_projection;
		public float4x4 view_projection_no_translation;
		public float4x4 inv_view;
		public float4x4 inv_projection;
		public float3 camera_position;
        public float2 window_resolution;
        public float delta_time;
		public float time;
	};
}

#include "types.slang"

struct Arguments {
	float4x4 view_matrix;
	
	float P00;
	float P11;
	
	float z_near;
	float z_far;
	
	float frustum[4];
	
	//float lod_base;
	//float lod_step;
	
	//float pyramid_width;
	//float pyramid_height;
	
	uint draw_count;

	/*
	int culling_enabled;
	int lod_enabled;
	int occlusion_enabled;
	int distance_check;
	int aabb_check;
	
	float aabb_x_min;
	float aabb_y_min;
	float aabb_z_min;
	float aabb_x_max;
	float aabb_y_max;
	float aabb_z_max;
	*/
	
	Instance *instance_buffer;
	Indirect *indirect_buffer;
	uint *final_instance_buffer;
};

[vk::push_constant]
Arguments args;

bool IsVisible(uint object_id)
{
	return true;
}

[shader("compute")]
[numthreads(256, 1, 1)]
void ComputeMain(uint3 tid : SV_DispatchThreadID)
{
	uint gid = tid.x;

	if (gid < args.draw_count) {
		uint object_id = args.instance_buffer[gid].object_id;
		bool visible = IsVisible(object_id);
		if (visible) {
			uint batch_index = args.instance_buffer[gid].batch_id;

			uint count;
			InterlockedAdd(args.indirect_buffer[batch_index].instance_count, 1, count);

			uint instance_index = args.indirect_buffer[batch_index].first_index + count;
			args.final_instance_buffer[instance_index] = object_id;
		}
	}
}

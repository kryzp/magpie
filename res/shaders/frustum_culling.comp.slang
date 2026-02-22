#include "types.slang"

/*
struct Arguments {
	float4x4 view_matrix;
	
	float P00;
	float P11;
	
	float z_near;
	float z_far;
	
	float frustum[4];
	
//	float lod_base;
//	float lod_step;
//	
//	float pyramid_width;
//	float pyramid_height;
	
	uint draw_count;

//	int culling_enabled;
//	int lod_enabled;
//	int occlusion_enabled;
//	int distance_check;
//	int aabb_check;
//	
//	float aabb_x_min;
//	float aabb_y_min;
//	float aabb_z_min;
//	float aabb_x_max;
//	float aabb_y_max;
//	float aabb_z_max;
	
	Instance *instance_buffer;
	Indirect *indirect_buffer;
	uint *final_instance_buffer;
};
*/

struct Arguments {
    ObjectData *objects;
    RenderMesh *meshes;
    PagePointers *pages;
    uint object_count;
};

[vk::push_constant]
Arguments args;

bool IsVisible(in ObjectData obj)
{
	return true;
}

bool IsTransparent(in ObjectData obj)
{
    return false;
}

IndirectDraw GetDrawFor(uint gid)
{
    ObjectData obj = args.objects[gid];
    RenderMesh mesh = args.meshes[obj.mesh_index];

    IndirectDraw draw;
    draw.index_count = mesh.index_count;
    draw.instance_count = 1;
    draw.first_index = mesh.first_index;
    draw.vertex_offset = 0; // We use BDA instead.
    draw.first_instance = gid;

    return draw;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void ComputeMain(uint3 tid : SV_DispatchThreadID)
{
	uint gid = tid.x;

    if (gid >= args.object_count)
        return;

    ObjectData obj = args.objects[gid];

    if (IsVisible(obj)) {
        PagePointers page = args.pages[obj.page_index];
        if (IsTransparent(obj)) {
            // Add to transparent pass.
        } else {
            // Add to opaque pass.
            uint slot;
            InterlockedAdd(*page.opaque_count_buffer, 1, slot);
            page.opaque_indirect_buffer[slot] = GetDrawFor(gid);
		}
	}
}

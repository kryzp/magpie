import types;
import culling;

struct Arguments {
    ObjectData *objects;
    RenderMesh *meshes;
    PagePointers *pages;
    IndirectDraw *indirect_buffer;
    uint *count_buffer;
    uint object_count; uint _padding;
    float4 sphere; // x,y,z = position | w = radius
};

[vk::push_constant]
Arguments args;

bool IsVisible(in ObjectData obj)
{
    float3 world_centre = mul(obj.model_matrix, float4(obj.sphere_bounds.xyz, 1.0)).xyz;

    float scale_x = length(obj.model_matrix[0].xyz);
    float scale_y = length(obj.model_matrix[1].xyz);
    float scale_z = length(obj.model_matrix[2].xyz);

    float max_scale = max(scale_x, max(scale_y, scale_z));

    float radius = obj.sphere_bounds.w * max_scale;

    float3 delta = world_centre - args.sphere.xyz;
    
    float dist_sq = dot(delta, delta);
    float radius_sum = radius + args.sphere.w;

	return dist_sq < radius_sum*radius_sum;
}

types::IndirectDraw GetDrawFor(uint gid)
{
    types::RenderMesh mesh = args.meshes[args.objects[gid].mesh_index];

    types::IndirectDraw draw;
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
        uint slot;
        InterlockedAdd(*args.count_buffer, 1, slot);
        args.indirect_buffer[slot] = culling::GetDrawFor(gid);
	}
}

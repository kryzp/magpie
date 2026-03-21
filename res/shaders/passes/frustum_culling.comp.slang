import types;
import culling;

struct Arguments {
    types::ObjectData *objects;
    types::RenderMesh *meshes;
    types::PagePointers *pages;
    types::IndirectDraw *indirect_buffer;
    uint *count_buffer;
    uint object_count; uint _padding;
    float4 frustum_planes[6]; // xyz = normal, w = distance
};

[vk::push_constant]
Arguments args;

bool IsVisible(in types.ObjectData obj)
{
    float3 world_centre = mul(obj.model_matrix, float4(obj.sphere_bounds.xyz, 1.0)).xyz;

    float scale_x = length(obj.model_matrix[0].xyz);
    float scale_y = length(obj.model_matrix[1].xyz);
    float scale_z = length(obj.model_matrix[2].xyz);

    float max_scale = max(scale_x, max(scale_y, scale_z));

    float radius = obj.sphere_bounds.w * max_scale;

    for (int i = 0; i < 6; i++) {
        float D = dot(args.frustum_planes[i].xyz, world_centre) + args.frustum_planes[i].w;
        if (D < -radius)
            return false;
    }

	return true;
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

    types::ObjectData obj = args.objects[gid];

    if (IsVisible(obj)) {
        types::PagePointers page = args.pages[obj.page_index];
        uint slot;
        InterlockedAdd(*args.count_buffer, 1, slot);
        args.indirect_buffer[slot] = GetDrawFor(gid);
	}
}

import scene;
import culling;

struct PushConstants
{
	scene::ObjectData *objects;
	scene::RenderMesh *meshes;
	scene::PagePointers *pages;
	scene::IndirectDraw *indirect_buffer;
	uint *count_buffer;
	uint object_count;
	uint _padding;
	float4 frustum_planes[6]; // xyz = normal, w = distance
};

[vk::push_constant]
PushConstants args;

[shader("compute")]
[numthreads(64, 1, 1)]
void ComputeMain(uint3 tid : SV_DispatchThreadID)
{
	uint gid = tid.x;
	if (gid >= args.object_count)
		return;

	scene::ObjectData obj = args.objects[gid];

	culling::FrustumTest test = culling::MakeFrustumTest(args.frustum_planes);

	if (culling::IsVisibleAgainst(obj, test))
	{
		scene::PagePointers page = args.pages[obj.page_index];
		uint slot;
		InterlockedAdd(*args.count_buffer, 1, slot);
		args.indirect_buffer[slot] = culling::GetDrawFor(args.objects, args.meshes, gid);
	}
}

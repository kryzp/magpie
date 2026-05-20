import scene;
import culling;

struct PushConstants
{
	scene::ObjectData *objects;
	scene::RenderMesh *meshes;
	scene::Material *materials;
	scene::PagePointers *pages;
	scene::IndirectDraw *indirect_buffer;
	uint32_t *count_buffer;
	uint32_t object_count;
	uint32_t alpha_filter;
	uint32_t max_draws_per_page;
	uint32_t _padding1;
	float4 sphere; // xyz = position, w = radius
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
	scene::Material mat = args.materials[obj.material_index];

	if (!culling::PassesAlphaFilter(mat.alpha_mode, args.alpha_filter))
		return;
	
	culling::SphereTest test = culling::MakeSphereTest(args.sphere);

	if (culling::IsVisibleAgainst(obj, test))
	{
		uint32_t page = obj.page_index;
		uint32_t slot;
		InterlockedAdd(args.count_buffer[page], 1, slot);
		uint32_t offset = page * args.max_draws_per_page;
		args.indirect_buffer[slot + offset] = culling::GetDrawFor(args.objects, args.meshes, gid);
	}
}

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
	uint32_t _padding0;
	uint32_t _padding1;
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
	scene::Material mat = args.materials[obj.material_index];

	if (!culling::PassesAlphaFilter(mat.alpha_mode, args.alpha_filter))
		return;

	culling::FrustumTest test = culling::MakeFrustumTest(args.frustum_planes);

	if (culling::IsVisibleAgainst(obj, test))
	{
		scene::PagePointers page = args.pages[obj.page_index];
		uint slot;
		InterlockedAdd(*args.count_buffer, 1, slot);
		args.indirect_buffer[slot] = culling::GetDrawFor(args.objects, args.meshes, gid);
	}
}

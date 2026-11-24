import scene;
import culling;
import shadows;

struct PushConstants {
	scene::ObjectData *objects;
	scene::RenderMesh *meshes;
	scene::Material *materials;
	shadows::ShadowCaster *casters;
	scene::IndirectDraw *indirect_buffer;
	uint32_t *count_buffer;
	uint32_t object_count;
	uint32_t caster_mask;
	uint32_t max_draws_per_page;
	uint32_t page_count;
};

[vk::push_constant]
PushConstants args;

[shader("compute")]
[numthreads(64, 1, 1)]
void ComputeMain(uint3 tid : SV_DispatchThreadID)
{
	uint gid = tid.x;
	uint caster_index = tid.y;

	if (gid >= args.object_count)
		return;

	// cached casters aren't re-rendered, so they don't need draws.
	if ((args.caster_mask & (1u << caster_index)) == 0)
		return;

	scene::ObjectData obj = args.objects[gid];
	scene::Material mat = args.materials[obj.material_index];

	// translucent objects don't cast (yet.........)
	if (mat.alpha_mode == scene::ALPHA_MODE_BLEND)
		return;

	shadows::ShadowCaster caster = args.casters[caster_index];

	culling::SphereTest test = culling::MakeSphereTest(float4(caster.position, caster.cull_radius));

	if (!culling::IsVisibleAgainst(obj, test))
		return;

	uint32_t stream_index = caster_index * shadows::SHADOW_STREAM_COUNT + ((mat.alpha_mode == scene::ALPHA_MODE_MASK) ? shadows::SHADOW_STREAM_MASKED : shadows::SHADOW_STREAM_OPAQUE);

	uint32_t counter_index = stream_index * args.page_count + obj.page_index;

	uint32_t slot;
	InterlockedAdd(args.count_buffer[counter_index], 1, slot);

	args.indirect_buffer[counter_index * args.max_draws_per_page + slot] = culling::GetDrawFor(args.objects, args.meshes, gid);
}

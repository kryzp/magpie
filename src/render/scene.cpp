#include "render/scene.h"
#include "graphics/device.h"
#include "graphics/bindless.h"

#include <string.h>

rnd::Scene::Scene(LogChannel log_channel_)
	: log_channel(log_channel_)
	, entity_pool(RENDER_SCENE_MAX_ENTITIES)
	, entity_count{}
	, frame_number(0)
	, last_sim_step_count(0)
	, gpu_completed_frame(0)
	, any_gpu_completed(false)
	, mesh_pool(RENDER_SCENE_MAX_MESHES)
	, mesh_high_water(0)
	, mesh_allocs{}
	, gpu_meshes{}
	, retired_region_count(0)
	, geometry_page_count(0)
	, material_pool(RENDER_SCENE_MAX_MATERIALS)
	, material_high_water(0)
	, gpu_materials{}
{
}

rnd::Scene::~Scene()
{
	gfx::Device *device = gfx::Device::Get();

	for (unsigned i = 0; i < geometry_page_count; i++) {
		device->BufferDestroy(geometry_pages[i].vertex_buffer);
		device->BufferDestroy(geometry_pages[i].index_buffer);
	}
}

void rnd::Scene::ReleaseRegion(const MeshAllocRegion &region)
{
	GeometryPage *page = &geometry_pages[region.page_index];

	page->vertex_free.Release(region.vertex_offset, region.vertex_count);
	page->index_free.Release(region.index_offset, region.index_count);

	page->vertex_count -= region.vertex_count;
	page->index_count -= region.index_count;
}

void rnd::Scene::BeginFrame(u64 frame_number_, bool has_completed, u64 gpu_completed_frame_)
{
	frame_number = frame_number_;
	any_gpu_completed = has_completed;
	gpu_completed_frame = gpu_completed_frame_;

	if (!has_completed)
		return;

	unsigned kept = 0;

	for (unsigned i = 0; i < retired_region_count; i++) {
		SceneRetiredRegion *r = &retired_regions[i];

		if (r->retire_frame <= gpu_completed_frame)
			ReleaseRegion(r->region);
		else
			retired_regions[kept++] = *r;
	}

	retired_region_count = kept;
}

rnd::EntityHandle rnd::Scene::EntityCreate(EntityType type)
{
	EntityHandle handle;
	handle.id = entity_pool.GetStableID();
	handle.type = type;

	unsigned index = entity_pool.IndexFromID(handle.id);

	entities[index] = Entity();
	entities[index].type = type;
	entity_count[type]++;

	return handle;
}

void rnd::Scene::EntityDestroy(EntityHandle handle)
{
	unsigned dense = entity_pool.IndexFromID(handle.id);
	unsigned moved = entity_pool.FreeID(handle.id);

	entities[dense] = entities[moved];
	entity_count[handle.type]--;
}

void rnd::Scene::SetObjectTransform(EntityHandle handle, const Mat4 &transform)
{
	DebugLogAssert(log_channel, handle.type == EntityType_Object, "Entity handle must point to object.");

	unsigned index = entity_pool.IndexFromID(handle.id);
	Object *object = &entities[index].object;

	if (!object->has_transform) {
		object->prev_transform = transform;
		object->has_transform = true;
	}

	object->transform = transform;
	object->normal_matrix = transform.Transposed().Inverse().WithoutTranslation();
}

void rnd::Scene::TeleportObject(EntityHandle handle, const Mat4 &transform)
{
	DebugLogAssert(log_channel, handle.type == EntityType_Object, "Entity handle must point to object.");

	unsigned index = entity_pool.IndexFromID(handle.id);
	Object *object = &entities[index].object;

	object->transform = transform;
	object->prev_transform = transform;
	object->has_transform = true;
	object->normal_matrix = transform.Transposed().Inverse().WithoutTranslation();
}

void rnd::Scene::BeginSimSteps(unsigned step_count)
{
	if (step_count == 0)
		return;

	for (unsigned i = 0; i < entity_pool.LiveCount(); i++) {
		Entity *entity = &entities[i];

		if (entity->type == EntityType_Object)
			entity->object.prev_transform = entity->object.transform;
	}

	last_sim_step_count = step_count;
}

Mat4 rnd::Scene::ObjectRenderTransform(EntityHandle handle, float interp_t) const
{
	unsigned index = entity_pool.IndexFromID(handle.id);
	const Object *object = &entities[index].object;
	return object->prev_transform.Lerp(object->transform, interp_t);
}

float rnd::Scene::InterpolationFactor(float alpha) const
{
	unsigned k = last_sim_step_count;

	if (k == 0)
		return 1.f;

	float t = ((float)k - 1.f + alpha) / (float)k;
	return maths::Clamp(t, 0.f, 1.f);
}

void rnd::Scene::SetObjectLocalSphereBounds(EntityHandle handle, const Vec4 &local_sphere_bounds)
{
	DebugLogAssert(log_channel, handle.type == EntityType_Object, "Entity handle must point to object.");

	unsigned index = entity_pool.IndexFromID(handle.id);

	entities[index].object.local_sphere_bounds = local_sphere_bounds;
}

void rnd::Scene::SetObjectMesh(EntityHandle handle, MeshHandle mesh)
{
	DebugLogAssert(log_channel, handle.type == EntityType_Object, "Entity handle must point to object.");

	unsigned index = entity_pool.IndexFromID(handle.id);

	entities[index].object.page_index = mesh.page_index;
	entities[index].object.mesh_index = mesh.slot_index;
}

void rnd::Scene::SetObjectMaterial(EntityHandle handle, MaterialHandle material)
{
	DebugLogAssert(log_channel, handle.type == EntityType_Object, "Entity handle must point to object.");

	unsigned index = entity_pool.IndexFromID(handle.id);

	entities[index].object.material_index = material.index;
}

void rnd::Scene::SetObjectSkinning(EntityHandle handle, const Mat4 *palette, unsigned joint_count)
{
	DebugLogAssert(log_channel, handle.type == EntityType_Object, "Entity handle must point to object.");

	unsigned index = entity_pool.IndexFromID(handle.id);

	entities[index].object.skinning_palette = palette;
	entities[index].object.skinning_joint_count = joint_count;
}

void rnd::Scene::SetLightParam(EntityHandle handle, const Light &light)
{
	DebugLogAssert(log_channel, handle.type == EntityType_Light, "Entity handle must point to light.");

	unsigned index = entity_pool.IndexFromID(handle.id);

	entities[index].light = light;
}

rnd::MeshHandle rnd::Scene::AllocMesh(const MeshDesc &desc)
{
	unsigned slot_index = 0;

	if (!mesh_pool.Alloc(&slot_index)) {
		DebugLogE(log_channel, "Unable to allocate more meshes!");
		return MeshHandle::Null();
	}

	unsigned page_index = FindSuitablePage(
		desc.vertex_count, desc.vertex_stride,
		desc.index_count, desc.index_type
	);

	GeometryPage *page = &geometry_pages[page_index];

	u64 vertex_offset = 0;
	u64 index_offset = 0;

	bool vok = page->vertex_free.TryAlloc(desc.vertex_count, &vertex_offset);
	bool iok = page->index_free.TryAlloc(desc.index_count, &index_offset);

	DebugLogAssert(log_channel, vok, "Vertex region allocation failed after GeometryFreeList::HasAvailable returned true.");
	DebugLogAssert(log_channel, iok, "Index region allocation failed after GeometryFreeList::HasAvailable returned true.");

	const u64 index_stride = gfx::IndexTypeStride(desc.index_type);

	gfx::Device *device = gfx::Device::Get();

	// removed because was bottlenecking see my explaination in CreateNewPage if you care
	/*
	{
		u8 *vertices_mapped = (u8 *)device->BufferMap(page->vertex_buffer);
		u8 *indices_mapped = (u8 *)device->BufferMap(page->index_buffer);

		memcpy(vertices_mapped + vertex_offset * desc.vertex_stride, desc.vertices, desc.vertex_count * desc.vertex_stride);
		memcpy(indices_mapped + index_offset * index_stride, desc.indices, desc.index_count * index_stride);
	}
	*/

	{
		const u64 vb_bytes = (u64)desc.vertex_count * desc.vertex_stride;
		const u64 ib_bytes = (u64)desc.index_count * index_stride;

		gfx::ResourceKey stage = device->StageAlloc(vb_bytes + ib_bytes);
		u8 *stage_mapped = (u8 *)device->BufferMap(stage);

		memcpy(stage_mapped, desc.vertices, vb_bytes);
		memcpy(stage_mapped + vb_bytes, desc.indices, ib_bytes);

		gfx::BufferCopy vb_copy = {};
		vb_copy.src_offset = 0;
		vb_copy.dst_offset = vertex_offset * desc.vertex_stride;
		vb_copy.size = vb_bytes;

		gfx::BufferCopy ib_copy = {};
		ib_copy.src_offset = vb_bytes;
		ib_copy.dst_offset = index_offset * index_stride;
		ib_copy.size = ib_bytes;

		gfx::CmdBuffer cmd = device->SubmitImBegin();

		if (vb_bytes > 0)
			cmd.CopyBufferToBuffer(stage, page->vertex_buffer, 1, &vb_copy);

		if (ib_bytes > 0)
			cmd.CopyBufferToBuffer(stage, page->index_buffer, 1, &ib_copy);

		VkMemoryBarrier2 barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;

		cmd.PipelineBarrier(0, 1, &barrier, 0, nullptr, 0, nullptr);

		device->SubmitImEnd(cmd);
		device->BufferDestroy(stage);
	}

	GPU_RenderMesh *gpu_mesh = &gpu_meshes[slot_index];
	gpu_mesh->index_count = desc.index_count;
	gpu_mesh->first_index = (unsigned)index_offset;
	gpu_mesh->vertex_buffer = device->BufferAddress(page->vertex_buffer) + (vertex_offset * desc.vertex_stride);

	if (!desc.skin_buffer.IsNull())
		gpu_mesh->skin_buffer = device->BufferAddress(desc.skin_buffer);
	else
		gpu_mesh->skin_buffer = 0;

	page->vertex_count += desc.vertex_count;
	page->index_count += desc.index_count;

	MeshAllocRegion *alloc = &mesh_allocs[slot_index];
	alloc->page_index = page_index;
	alloc->vertex_offset = vertex_offset;
	alloc->vertex_count = desc.vertex_count;
	alloc->index_offset = index_offset;
	alloc->index_count = desc.index_count;

	mesh_high_water = maths::Max(mesh_high_water, slot_index + 1);

	MeshHandle handle;
	handle.slot_index = slot_index;
	handle.page_index = page_index;

	return handle;
}

void rnd::Scene::FreeMesh(MeshHandle handle)
{
	DebugLogAssert(
		log_channel,
		retired_region_count < ArraySize(retired_regions),
		"Too many mesh regions waiting to retire."
	);

	SceneRetiredRegion *retired = &retired_regions[retired_region_count++];
	retired->region = mesh_allocs[handle.slot_index];
	retired->retire_frame = frame_number;

	gpu_meshes[handle.slot_index] = GPU_RenderMesh();
	mesh_allocs[handle.slot_index] = MeshAllocRegion();

	mesh_pool.Free(handle.slot_index);
}

unsigned rnd::Scene::CountOfMeshes() const
{
	return mesh_pool.LiveCount();
}

unsigned rnd::Scene::FindSuitablePage(
	unsigned vertex_count, u64 vertex_stride,
	unsigned index_count, gfx::IndexType index_type
)
{
	for (unsigned i = 0; i < geometry_page_count; i++) {
		GeometryPage *page = &geometry_pages[i];

		if (page->vertex_free.HasAvailable(vertex_count) &&
			page->index_free.HasAvailable(index_count) &&
			page->vertex_stride == vertex_stride &&
			page->index_type == index_type) {
			return i;
		}
	}

	DebugLogAssert(
		log_channel,
		geometry_page_count < ArraySize(geometry_pages),
		"Exhausted all possible geometry pages."
	);

	unsigned new_index = geometry_page_count;

	geometry_pages[new_index] = CreateNewPage(vertex_stride, index_type);
	geometry_page_count++;

	return new_index;
}

rnd::GeometryPage rnd::Scene::CreateNewPage(u64 vertex_stride, gfx::IndexType index_type)
{
	DebugLogD(log_channel, "Creating a new geometry page... VERTEXSTRIDE: %llu, INDEXTYPE: %u", (u64)vertex_stride, (unsigned)index_type);


	// tpc im so pissed off ight now.
	// ok so:
	// depth prepass and forward pass cost a) absurdly too much and b) suspiciously the same amount
	// so I've been searching around for why and eventually I realized that they are
	// bottlenecked by the bus moving data from the scene pages, which also explained why the shadow pass was also
	// being super slow.
	// anyway the fix is to remove the host access flags so that the buffers here go into VRAM
	// because VMA prefers device-local + host-visible memory first and without resizable BAR the heap is only around 256 MB.
	// and since my pages are so big they end up splitting up host memory, so from then on every vertex the GPU pulls
	// through BDA goes over PCIe which is super slow and we wanna avoid that.

	// essentially:
	// VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT -> must give me HOST_VISIBLE memory
	// but typically having HOST_VISIBLE memory and DEVICE_LOCAL memory is mutually exclusive, except for the small window called the BAR.
	// so we end up having to store the rest of it elsewhere and access it via PCIe in every shader which sucks balls


	// we use vertex pulling so don't need to use VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT.
	gfx::BufferAllocInfo vb_info = {};
	vb_info.flags = 0;//VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vb_info.size = RENDER_GEOMETRY_PAGE_VERTEX_BUFFER_SIZE;
	vb_info.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	vb_info.usage |= VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	//vb_info.usage |= VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

	gfx::BufferAllocInfo ib_info = {};
	ib_info.flags = 0;//VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ib_info.size = RENDER_GEOMETRY_PAGE_INDEX_BUFFER_SIZE;
	ib_info.usage |= VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	ib_info.usage |= VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	ib_info.usage |= VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT;
	//ib_info.usage |= K_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KH;

	u64 index_stride = gfx::IndexTypeStride(index_type);

	unsigned max_vertices = (unsigned)(vb_info.size / vertex_stride);
	unsigned max_indices = (unsigned)(ib_info.size / index_stride);

	gfx::Device *device = gfx::Device::Get();

	GeometryPage page;
	page.vertex_buffer = device->BufferAlloc(vb_info);
	page.index_buffer = device->BufferAlloc(ib_info);
	page.vertex_count = 0;
	page.index_count = 0;
	page.vertex_stride = vertex_stride;
	page.index_type = index_type;
	page.max_vertices = max_vertices;
	page.max_indices = max_indices;
	page.vertex_free = GeometryFreeList(max_vertices);
	page.index_free = GeometryFreeList(max_indices);

	return page;
}

rnd::MaterialHandle rnd::Scene::AddMaterial(const Material &material)
{
	unsigned slot_index = 0;

	if (!material_pool.Alloc(&slot_index)) {
		DebugLogE(log_channel, "Unable to allocate more materials!");
		return MaterialHandle::Null();
	}

	cpu_materials[slot_index] = material;

	BakeMaterialIntoGPU(material, &gpu_materials[slot_index]);
	material_high_water = maths::Max(material_high_water, slot_index + 1);

	MaterialHandle handle;
	handle.index = slot_index;

	return handle;
}

rnd::MaterialHandle rnd::Scene::AddMaterialFromAssets(const ast::ModelMaterial &source)
{
	Material render_material = Material::FromAsset(source);
	return AddMaterial(render_material);
}

void rnd::Scene::UpdateMaterial(MaterialHandle handle, const Material &material)
{
	cpu_materials[handle.index] = material;
	BakeMaterialIntoGPU(material, &gpu_materials[handle.index]);
}

void rnd::Scene::FreeMaterial(MaterialHandle handle)
{
	gpu_materials[handle.index] = GPU_Material();
	material_pool.Free(handle.index);
}

unsigned rnd::Scene::CountOfMaterials() const
{
	return material_pool.LiveCount();
}

const rnd::Material *rnd::Scene::GetMaterial(MaterialHandle handle) const
{
	return &cpu_materials[handle.index];
}

static unsigned ResolveToBindlessIndexInternal(gfx::ResourceKey key)
{
	if (key.IsNull())
		return GRAPHICS_BINDLESS_INDEX_INVALID;

	gfx::Device *device = gfx::Device::Get();

	return device->TextureViewBindless(device->TextureViewAuto(key));
}

void rnd::Scene::BakeMaterialIntoGPU(const Material &material, GPU_Material *out) const
{
	out->albedo_texture = ResolveToBindlessIndexInternal(material.albedo_texture);
	out->normal_texture = ResolveToBindlessIndexInternal(material.normal_texture);
	out->metallic_roughness_texture = ResolveToBindlessIndexInternal(material.metallic_roughness_texture);
	out->emissive_texture = ResolveToBindlessIndexInternal(material.emissive_texture);
	out->occlusion_texture = ResolveToBindlessIndexInternal(material.occlusion_texture);

	out->albedo_factor = material.albedo_factor;
	out->normal_scale = material.normal_scale;
	out->metallic_factor = material.metallic_factor;
	out->roughness_factor = material.roughness_factor;
	out->emissive_factor = material.emissive_factor;
	out->emissive_intensity = material.emissive_intensity;
	out->occlusion_intensity = material.occlusion_intensity;

	out->ior = material.ior;

	out->transmission_texture = ResolveToBindlessIndexInternal(material.transmission_texture);
	out->thickness_texture = ResolveToBindlessIndexInternal(material.thickness_texture);

	out->transmission_factor = material.transmission_factor;
	out->thickness_factor = material.thickness_factor;

	out->attenuation_colour = material.attenuation_colour;
	out->attenuation_distance = material.attenuation_distance;

	out->specular_texture = ResolveToBindlessIndexInternal(material.specular_texture);
	out->specular_colour_texture = ResolveToBindlessIndexInternal(material.specular_colour_texture);

	out->specular_factor = material.specular_factor;
	out->specular_colour_factor = material.specular_colour_factor;

	out->clearcoat_texture = ResolveToBindlessIndexInternal(material.clearcoat_texture);
	out->clearcoat_roughness_texture = ResolveToBindlessIndexInternal(material.clearcoat_roughness_texture);

	out->clearcoat_factor = material.clearcoat_factor;
	out->clearcoat_roughness_factor = material.clearcoat_roughness_factor;

	out->sheen_colour_texture = ResolveToBindlessIndexInternal(material.sheen_colour_texture);
	out->sheen_roughness_texture = ResolveToBindlessIndexInternal(material.sheen_roughness_texture);

	out->sheen_colour_factor = material.sheen_colour_factor;
	out->sheen_roughness_factor = material.sheen_roughness_factor;

	out->iridescence_texture = ResolveToBindlessIndexInternal(material.iridescence_texture);
	out->iridescence_thickness_texture = ResolveToBindlessIndexInternal(material.iridescence_thickness_texture);

	out->iridescence_factor = material.iridescence_factor;
	out->iridescence_ior = material.iridescence_ior;
	out->iridescence_thickness_min_nanometers = material.iridescence_thickness_min_nanometers;
	out->iridescence_thickness_max_nanometers = material.iridescence_thickness_max_nanometers;

	out->double_sided = material.double_sided;
	out->unlit = material.unlit;

	out->alpha_cutoff = material.alpha_cutoff;
	out->alpha_mode = material.alpha_mode;

	out->gloss_in_specular_alpha = material.gloss_in_specular_alpha ? 1 : 0;
}

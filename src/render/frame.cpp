#include "render/frame.h"
#include "render/system.h"
#include "render/pass/shadow_pass.h"
#include "graphics/device.h"
#include "asset/asset_system.h"

#include <string.h>

rnd::FramePacket::FramePacket()
	: frame_number(0)
	, dt(0.f)
	, elapsed(0.f)
	, window_width(0)
	, window_height(0)
	, page_count(0)
{
}

void rnd::FramePacket::Begin(u64 frame_number_, float dt_, float elapsed_, unsigned window_width_, unsigned window_height_)
{
	frame_number = frame_number_;
	dt = dt_;
	elapsed = elapsed_;

	window_width = window_width_;
	window_height = window_height_;

	camera = Camera();

	objects.clear();
	skinning_matrices.clear();
	lights.clear();
	meshes.clear();
	materials.clear();

	page_count = 0;

	for (unsigned mode = 0; mode < DebugDepthMode_MaxEnum; mode++) {
		debug.instances[mode].clear();
		debug.batch_count[mode] = 0;
	}
}

void rnd::FramePacket::CollectScene(const Scene &scene, const Camera &camera_, float interp_t)
{
	camera = camera_;

	meshes.assign(scene.GPUMeshes(), scene.GPUMeshes() + scene.MeshHighWater());
	materials.assign(scene.GPUMaterials(), scene.GPUMaterials() + scene.MaterialHighWater());

	page_count = scene.PageCount();

	gfx::Device *device = gfx::Device::Get();

	for (unsigned i = 0; i < page_count; i++) {
		const GeometryPage &page = scene.PageAt(i);
		pages[i].vertex_buffer_address = device->BufferAddress(page.vertex_buffer);
		pages[i].index_buffer = page.index_buffer;
		pages[i].index_type = page.index_type;
	}

	unsigned object_count = 0;
	unsigned light_count = 0;
	unsigned joint_total = 0;

	for (unsigned i = 0; i < scene.EntityLiveCount(); i++) {
		const Entity &entity = scene.EntityAt(i);

		if (entity.type == EntityType_Object) {
			object_count++;
			if (entity.object.skinning_palette)
				joint_total += entity.object.skinning_joint_count;
		} else if (entity.type == EntityType_Light) {
			light_count++;
		}
	}

	objects.reserve(object_count);
	lights.reserve(light_count);
	skinning_matrices.reserve(joint_total);

	for (unsigned i = 0; i < scene.EntityLiveCount(); i++) {
		const Entity &entity = scene.EntityAt(i);

		if (entity.type == EntityType_Light) {
			lights.push_back(entity.light);
			continue;
		}

		if (entity.type != EntityType_Object)
			continue;

		const Object &object = entity.object;

		objects.push_back(PacketObject());
		PacketObject *out = &objects.back();

		// only rebuild if moved
		if (interp_t < 1.f && object.prev_transform != object.transform) {
			out->transform = object.prev_transform.Lerp(object.transform, interp_t);
			out->normal_matrix = out->transform.Transposed().Inverse().WithoutTranslation();
		} else {
			out->transform = object.transform;
			out->normal_matrix = object.normal_matrix;
		}

		out->local_sphere_bounds = object.local_sphere_bounds;
		out->page_index = object.page_index;
		out->mesh_index = object.mesh_index;
		out->material_index = object.material_index;
		out->flags = ObjectFlag_None;

		if (object.material_index < materials.size() &&
			materials[object.material_index].alpha_mode == ast::AlphaMode_Blend) {
			out->flags |= ObjectFlag_Translucent;
		}

		if (object.skinning_palette && object.skinning_joint_count > 0) {
			out->skinning_first_matrix = skinning_matrices.size();
			out->skinning_joint_count = object.skinning_joint_count;

			skinning_matrices.insert(
				skinning_matrices.end(),
				object.skinning_palette,
				object.skinning_palette + object.skinning_joint_count
			);
		}
	}
}

void rnd::FrameParams::UploadSceneTables(const FramePacket &packet, gfx::RingBuffer *upload)
{
	mesh_table = upload->PushArray<GPU_RenderMesh>(maths::Max<u64>(packet.meshes.size(), 1));
	material_table = upload->PushArray<GPU_Material>(maths::Max<u64>(packet.materials.size(), 1));

	if (mesh_table.cpu && !packet.meshes.empty())
		memcpy(mesh_table.cpu, packet.meshes.data(), packet.meshes.size() * sizeof(GPU_RenderMesh));

	if (material_table.cpu && !packet.materials.empty())
		memcpy(material_table.cpu, packet.materials.data(), packet.materials.size() * sizeof(GPU_Material));

	page_count = packet.page_count;
	page_table_buffer = upload->PushArray<GPU_PagePointers>(maths::Max<unsigned>(packet.page_count, 1));

	GPU_PagePointers *mapped = (GPU_PagePointers *)page_table_buffer.cpu;

	for (unsigned i = 0; mapped && i < packet.page_count; i++) {
		mapped[i].vertex_buffer = packet.pages[i].vertex_buffer_address;
		page_index_buffers[i] = packet.pages[i].index_buffer;
		page_index_types[i] = packet.pages[i].index_type;
	}
}

void rnd::FrameParams::UploadObjects(const FramePacket &packet, gfx::RingBuffer *upload)
{
	object_count = packet.objects.size();

	if (!packet.skinning_matrices.empty())
		skinning_palette_buffer = upload->PushCopyArray<Mat4>(packet.skinning_matrices.data(), packet.skinning_matrices.size());

	object_buffer = upload->PushArray<GPU_ObjectData>(maths::Max<unsigned>(object_count, 1));

	GPU_ObjectData *mapped = (GPU_ObjectData *)object_buffer.cpu;

	for (unsigned i = 0; mapped && i < object_count; i++) {
		const PacketObject *object = &packet.objects[i];

		mapped[i].model_matrix = object->transform;
		mapped[i].normal_matrix = object->normal_matrix;
		mapped[i].local_sphere_bounds = object->local_sphere_bounds;
		mapped[i].material_index = object->material_index;
		mapped[i].mesh_index = object->mesh_index;
		mapped[i].page_index = object->page_index;
		mapped[i].skinning_joint_count = object->skinning_joint_count;
		mapped[i].skinning_palette_buffer = 0;

		if (object->skinning_joint_count > 0)
			mapped[i].skinning_palette_buffer = skinning_palette_buffer.gpu + object->skinning_first_matrix * sizeof(Mat4);
	}
}

void rnd::FrameParams::UploadLights(const FramePacket &packet, gfx::RingBuffer *upload)
{
	light_count = packet.lights.size();
	light_buffer = upload->PushArray<GPU_Light>(maths::Max<unsigned>(light_count, 1));

	GPU_Light *mapped = (GPU_Light *)light_buffer.cpu;

	for (unsigned i = 0; mapped && i < light_count; i++) {
		const Light *light = &packet.lights[i];

		const float radius = light->HeuristicRadius(0.05f);

		mapped[i].transform = Mat4::Transform(light->position, Quat::Identity(), Vec3(radius), Vec3(0.f));
		mapped[i].position = light->position;
		mapped[i].colour = light->colour;
		mapped[i].intensity = light->intensity;
		mapped[i].attenuation = Vec3(light->falloff, 0.f, 0.f);
		mapped[i].radius = radius;
		mapped[i].shadow_slot_index = -1;

		if (light->casts_shadows && shadow_caster_count < ArraySize(shadow_casters)) {
			mapped[i].shadow_slot_index = shadow_caster_count;

			ShadowCaster *caster = &shadow_casters[shadow_caster_count++];
			caster->position = light->position;
			caster->near_plane = light->shadow_near;
			caster->far_plane = light->shadow_far;
			caster->radius = radius;
		}
	}
}

void rnd::FrameParams::UploadFrameData(const FramePacket &packet, gfx::RingBuffer *upload)
{
	frame_data = upload->PushArray<GPU_FrameData>(1);

	GPU_FrameData *data = (GPU_FrameData *)frame_data.cpu;

	if (!data)
		return;

	data->view = camera.view;
	data->proj = camera.proj;
	data->view_proj = camera.view_proj;
	data->view_proj_no_translation = camera.view_proj_no_translation;
	data->inv_view = camera.inv_view;
	data->inv_proj = camera.inv_proj;
	data->camera_position = camera.position;
	data->window_resolution = Vec2(packet.window_width, packet.window_height);
	data->frame_number = frame_number;
	data->delta_time = dt;
	data->time = elapsed;
}

static bool TryResolveShader(ast::Handle handle, gfx::ResourceKey *out)
{
	ast::Record *record = ast::AssetSystem::Get()->GetRecord(handle);

	if (!record || record->load_state != ast::LoadState_Ready)
		return false;

	*out = record->asset.shader.key;
	return !out->IsNull();
}

bool rnd::FrameParams::ResolveShaders(const System *system)
{
	bool ok = true;

	ok &= TryResolveShader(system->shaders.debug_line_handle, &debug_line_shader);
	ok &= TryResolveShader(system->shaders.depth_prepass_handle, &depth_prepass_shader);
	ok &= TryResolveShader(system->shaders.forward_handle, &forward_shader);
	ok &= TryResolveShader(system->shaders.shadow_handle, &shadow_shader);
	ok &= TryResolveShader(system->shaders.shadow_masked_handle, &shadow_masked_shader);
	ok &= TryResolveShader(system->shaders.shadow_cull_handle, &shadow_cull_shader);
	ok &= TryResolveShader(system->shaders.cull_frustum_handle, &cull_frustum_shader);
	ok &= TryResolveShader(system->shaders.skybox_handle, &skybox_shader);
	ok &= TryResolveShader(system->shaders.tonemapping_handle, &tonemapping_shader);
	ok &= TryResolveShader(system->shaders.brdf_lut_generation_handle, &brdf_lut_generation_shader);
	ok &= TryResolveShader(system->shaders.hdr_to_cubemap_handle, &hdr_to_cubemap_shader);
	ok &= TryResolveShader(system->shaders.irradiance_cubemap_gen_handle, &irradiance_cubemap_gen_shader);
	ok &= TryResolveShader(system->shaders.prefilter_cubemap_gen_handle, &prefilter_cubemap_gen_shader);

	return ok;
}

struct TranslucentSortKey {
	float depth;
	unsigned index;
};

static void SortTranslucentKeysFarToNear(TranslucentSortKey *keys, unsigned count)
{
	// shell sort
	// todo: is this actually good enough - test

	static const int gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1 };

	for (int g = 0; g < ArraySize(gaps); g++) {
		int gap = gaps[g];

		for (int i = gap; i < count; i++) {
			TranslucentSortKey key = keys[i];
			unsigned j = i;

			while (j >= gap && keys[j - gap].depth < key.depth) {
				keys[j] = keys[j - gap];
				j -= gap;
			}

			keys[j] = key;
		}
	}
}

static bool FrustumTestSphere(const rnd::FrustumVolume &frustum, const Vec3 &centre, float radius)
{
	for (int i = 0; i < 6; i++) {
		const Vec4 &p = frustum.planes[i];
		if (p.x * centre.x + p.y * centre.y + p.z * centre.z + p.w < -radius)
			return false;
	}

	return true;
}

void rnd::FrameParams::BuildTranslucentList(const FramePacket &packet)
{
	translucent_draws.clear();

	unsigned candidate_count = 0;

	for (const PacketObject &object : packet.objects) {
		if (object.flags & ObjectFlag_Translucent)
			candidate_count++;
	}

	if (candidate_count == 0)
		return;

	FrustumVolume frustum = camera.Frustum();

	Vector<TranslucentSortKey> keys;
	keys.reserve(candidate_count);

	for (unsigned i = 0; i < packet.objects.size(); i++) {
		const PacketObject *object = &packet.objects[i];

		if (!(object->flags & ObjectFlag_Translucent))
			continue;

		// same bound as the culling shader.
		// centre through the model matrix radius by the largest axis scale.
		// todo: should this be shared behaviour?
		const Vec4 &b = object->local_sphere_bounds;
		Vec3 centre = object->transform.TransformPoint(Vec3(b.x, b.y, b.z));

		float sx = Vec3(object->transform.m00, object->transform.m10, object->transform.m20).Length();
		float sy = Vec3(object->transform.m01, object->transform.m11, object->transform.m21).Length();
		float sz = Vec3(object->transform.m02, object->transform.m12, object->transform.m22).Length();
		float radius = b.w * maths::Max(sx, maths::Max(sy, sz));

		if (!FrustumTestSphere(frustum, centre, radius))
			continue;

		TranslucentSortKey key;
		key.depth = (centre - camera.position).Dot(camera.forward);
		key.index = i;
		keys.push_back(key);
	}

	SortTranslucentKeysFarToNear(keys.data(), keys.size());

	for (const TranslucentSortKey &key : keys) {
		const PacketObject *object = &packet.objects[key.index];

		if (object->mesh_index >= packet.meshes.size())
			continue;

		const GPU_RenderMesh *mesh = &packet.meshes[object->mesh_index];

		TranslucentDraw draw;
		draw.object_index = key.index;
		draw.page_index = object->page_index;
		draw.index_count = mesh->index_count;
		draw.first_index = mesh->first_index;

		translucent_draws.push_back(draw);
	}
}

void rnd::FrameParams::BuildShared(System *system)
{
	brdf_lut = system->brdf_lut;
	irradiance_fallback_cubemap = system->irradiance_cubemap;
	prefilter_cubemap = system->prefilter_cubemap;

	skybox_mesh = &system->skybox_mesh;

	cubemap_capture_transform_buffer = system->cubemap_capture_transform_buffer;

	linear_sampler = system->samplers.linear;
	nearest_sampler = system->samplers.nearest;
	material_sampler = system->samplers.material;
	shadow_compare_sampler = system->samplers.shadow_compare;
}

void rnd::FrameParams::Build(System *system, gfx::RingBuffer *upload_, const FramePacket *packet_)
{
	upload = upload_;
	packet = packet_;
	frame_number = packet->frame_number;
	dt = packet->dt;
	elapsed = packet->elapsed;
	camera = packet->camera;

	skinning_palette_buffer = gfx::Alloc();
	shadow_caster_count = 0;

	BuildShared(system);

	UploadSceneTables(*packet, upload);
	UploadObjects(*packet, upload);
	UploadLights(*packet, upload);
	UploadFrameData(*packet, upload);
	system->shadow_renderer.BuildCasterTable(upload, this);
	BuildTranslucentList(*packet);

	shaders_ready = ResolveShaders(system);
}

void rnd::FrameParams::RecordIndirect(
	const gfx::CmdBuffer &cmd,
	gfx::ResourceKey indirect_buffer,
	gfx::ResourceKey count_buffer
) const
{
	RecordIndirectStream(cmd, indirect_buffer, count_buffer, 0);
}

void rnd::FrameParams::RecordIndirectStream(
	const gfx::CmdBuffer &cmd,
	gfx::ResourceKey indirect_buffer,
	gfx::ResourceKey count_buffer,
	u64 stream_index
) const
{
	const u64 max_draws_per_page = RENDER_SCENE_MAX_ENTITIES;
	const u64 first_slot = stream_index * page_count;

	for (unsigned i = 0; i < page_count; i++) {
		u64 slot = first_slot + i;
		u64 indirect_offset = slot * sizeof(GPU_IndirectDraw) * max_draws_per_page;
		u64 count_offset = slot * sizeof(unsigned);

		cmd.BindIndexBuffer(
			page_index_buffers[i],
			0, VK_WHOLE_SIZE,
			page_index_types[i]
		);

		cmd.DrawIndexedIndirectCount(
			indirect_buffer,
			indirect_offset,
			count_buffer,
			count_offset,
			max_draws_per_page,
			sizeof(GPU_IndirectDraw)
		);
	}
}

void rnd::FrameParams::RecordTranslucent(const gfx::CmdBuffer &cmd) const
{
	unsigned bound_page = (unsigned)-1;

	for (const TranslucentDraw &draw : translucent_draws) {
		if (draw.page_index >= page_count)
			continue;

		if (draw.page_index != bound_page) {
			bound_page = draw.page_index;

			cmd.BindIndexBuffer(
				page_index_buffers[bound_page],
				0, VK_WHOLE_SIZE,
				page_index_types[bound_page]
			);
		}

		cmd.DrawIndexed(
			draw.index_count,
			1,
			draw.first_index,
			0,
			draw.object_index
		);
	}
}

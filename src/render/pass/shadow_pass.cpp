#include "render/pass/shadow_pass.h"
#include "render/graph/graph.h"
#include "graphics/device.h"
#include "core/hash.h"

#include <cmath>

struct ShadowPushConstants {
	u64 object_buffer;
	u64 mesh_buffer;
	u64 material_buffer;
	u64 caster_buffer;
	u32 caster_index;
	u32 material_sampler;
};

static void SetupShadowPipeline(gfx::GraphicsPipelineDef *def)
{
	def->depth_stencil_state.depth_test_enabled = true;
	def->depth_stencil_state.depth_write_enabled = true;
	def->depth_stencil_state.depth_compare_op = VK_COMPARE_OP_GREATER; // reversed-z

	def->depth_bias_enabled = true;
	def->depth_bias_constant = RENDER_SHADOW_DEPTH_BIAS_CONSTANT;
	def->depth_bias_slope = RENDER_SHADOW_DEPTH_BIAS_SLOPE;
}

void rnd::ShadowMappingPassFn(const PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const ShadowMappingPassData *data = (const ShadowMappingPassData *)ctx.user_data;
	const FrameParams *frame_params = data->frame_params;
	gfx::Device *device = gfx::Device::Get();

	gfx::ResourceKey indirect_key = ctx.graph->ResolveBuffer(data->draw_stream.indirect_buffer);
	gfx::ResourceKey counter_key = ctx.graph->ResolveBuffer(data->draw_stream.count_buffer);

	ShadowPushConstants pc = {};
	pc.object_buffer = frame_params->object_buffer.gpu;
	pc.mesh_buffer = frame_params->mesh_table.gpu;
	pc.material_buffer = frame_params->material_table.gpu;
	pc.caster_buffer = frame_params->shadow_caster_table.gpu;
	pc.caster_index = data->caster_index;
	pc.material_sampler = device->SamplerBindless(frame_params->material_sampler);

	// opaque
	{
		gfx::GraphicsPipelineDef pipeline_def(frame_params->shadow_shader, *ctx.render_info);
		SetupShadowPipeline(&pipeline_def);

		gfx::PipelineState pipeline_st = device->FetchGraphicsPipeline(pipeline_def);

		cmd.BindPipelineState(pipeline_st);
		cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, pc, 0);

		frame_params->RecordIndirectStream(cmd, indirect_key, counter_key, ShadowStreamIndex(data->caster_index, ShadowStream_Opaque));
	}

	// alpha masked
	{
		gfx::GraphicsPipelineDef pipeline_def(frame_params->shadow_masked_shader, *ctx.render_info);
		SetupShadowPipeline(&pipeline_def);

		// turn off culling 'cause most instances of these
		// objects are going to be double-sided flat cards
		pipeline_def.cull_mode = VK_CULL_MODE_NONE;

		gfx::PipelineState pipeline_st = device->FetchGraphicsPipeline(pipeline_def);

		cmd.BindPipelineState(pipeline_st);
		cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, pc, 0);

		frame_params->RecordIndirectStream(cmd, indirect_key, counter_key, ShadowStreamIndex(data->caster_index, ShadowStream_Masked));
	}
}

rnd::ShadowRenderer::ShadowRenderer()
{
	gfx::Device *device = gfx::Device::Get();

	for (int i = 0; i < RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS; i++) {
		shadow_cubemaps[i] = device->TextureAllocCubemapDepth(RENDER_SHADOW_MAP_RESOLUTION, 1, RENDER_SHADOW_MAP_FORMAT);

		gfx::TextureViewCreateInfo view_info;
		view_info.texture = shadow_cubemaps[i];
		view_info.type = VK_IMAGE_VIEW_TYPE_CUBE;
		view_info.range = gfx::SubresourceRange::AllDepth();

		shadow_cubemap_views[i] = device->TextureViewFetch(view_info);
	}
}

rnd::ShadowRenderer::~ShadowRenderer()
{
	gfx::Device *device = gfx::Device::Get();

	for (int i = 0; i < RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS; i++)
		device->TextureDestroy(shadow_cubemaps[i]);
}

void rnd::ShadowRenderer::InvalidateCache()
{
	for (int i = 0; i < RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS; i++)
		cache_valid[i] = false;
}

// just a copy of culling::IsVisibleAgainst
// todo: ???
static bool ObjectTouchesSphere(const rnd::PacketObject &object, const Vec3 &centre, float radius)
{
	const Vec4 &b = object.local_sphere_bounds;
	const Mat4 &m = object.transform;

	Vec3 object_centre = m.TransformPoint(Vec3(b.x, b.y, b.z));

	float sx = Vec3(m.m00, m.m10, m.m20).Length();
	float sy = Vec3(m.m01, m.m11, m.m21).Length();
	float sz = Vec3(m.m02, m.m12, m.m22).Length();
	float object_radius = b.w * maths::Max(sx, maths::Max(sy, sz));

	// inflate so that the CPU set is always a superset of the GPU set
	float reach = (object_radius + radius) * 1.001f + 0.001f;

	return (object_centre - centre).LengthSqr() < reach * reach;
}

static u64 HashCasterContents(const rnd::FramePacket &packet, const rnd::ShadowCaster &caster, float cull_radius)
{
	u64 h = hash::FNV_OFFSET_BASIS;

	constexpr unsigned resolution = RENDER_SHADOW_MAP_RESOLUTION;
	constexpr VkFormat format = RENDER_SHADOW_MAP_FORMAT;

	h = hash::BytesCombine(h, &resolution, sizeof(resolution));
	h = hash::BytesCombine(h, &format, sizeof(format));
	h = hash::BytesCombine(h, &caster.position, sizeof(caster.position));
	h = hash::BytesCombine(h, &caster.near_plane, sizeof(caster.near_plane));
	h = hash::BytesCombine(h, &caster.far_plane, sizeof(caster.far_plane));
	h = hash::BytesCombine(h, &cull_radius, sizeof(cull_radius));

	for (const rnd::PacketObject &object : packet.objects) {
		if (object.flags & rnd::ObjectFlag_Translucent)
			continue; // blended objects never cast

		if (!ObjectTouchesSphere(object, caster.position, cull_radius))
			continue;

		h = hash::BytesCombine(h, &object.transform, sizeof(object.transform));
		h = hash::BytesCombine(h, &object.page_index, sizeof(object.page_index));
		h = hash::BytesCombine(h, &object.mesh_index, sizeof(object.mesh_index));
		h = hash::BytesCombine(h, &object.material_index, sizeof(object.material_index));

		if (object.mesh_index < packet.meshes.size()) {
			const rnd::GPU_RenderMesh &mesh = packet.meshes[object.mesh_index];
			h = hash::BytesCombine(h, &mesh, sizeof(mesh));
		}

		if (object.material_index < packet.materials.size()) {
			const rnd::GPU_Material &mat = packet.materials[object.material_index];

			h = hash::BytesCombine(h, &mat.alpha_mode, sizeof(mat.alpha_mode));

			if (mat.alpha_mode == ast::AlphaMode_Mask) {
				// albedo alpha + alpha cutoff affects the depth of the object
				h = hash::BytesCombine(h, &mat.albedo_texture, sizeof(mat.albedo_texture));
				h = hash::BytesCombine(h, &mat.albedo_factor, sizeof(mat.albedo_factor));
				h = hash::BytesCombine(h, &mat.alpha_cutoff, sizeof(mat.alpha_cutoff));
			}
		}

		// skinned meshes change shape whenever the pose does
		if (object.skinning_joint_count > 0 && object.skinning_first_matrix + object.skinning_joint_count <= packet.skinning_matrices.size())
		{
			h = hash::BytesCombine(
				h,
				&packet.skinning_matrices[object.skinning_first_matrix],
				object.skinning_joint_count * sizeof(Mat4)
			);
		}
	}

	return h;
}

void rnd::ShadowRenderer::BuildCasterTable(gfx::RingBuffer *upload, FrameParams *frame_params) const
{
	static const Vec3 light_dirs[6] = {
		Vec3(1.f, 0.f, 0.f),  // Right.
		Vec3(-1.f, 0.f, 0.f), // Left.
		Vec3(0.f, 0.f, 1.f),  // Up.
		Vec3(0.f, 0.f, -1.f), // Down.
		Vec3(0.f, 1.f, 0.f),  // Forward.
		Vec3(0.f, -1.f, 0.f), // Backwards.
	};

	static const Vec3 light_ups[6] = {
		Vec3(0.f, 0.f, 1.f),  // Right.
		Vec3(0.f, 0.f, 1.f),  // Left.
		Vec3(0.f, -1.f, 0.f), // Up.
		Vec3(0.f, 1.f, 0.f),  // Down.
		Vec3(0.f, 0.f, 1.f),  // Forward.
		Vec3(0.f, 0.f, 1.f),  // Backwards.
	};

	gfx::Device *device = gfx::Device::Get();

	unsigned caster_count = maths::Min<unsigned>(frame_params->shadow_caster_count, RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS);

	frame_params->shadow_caster_table = upload->PushArray<GPU_ShadowCaster>(maths::Max<unsigned>(caster_count, 1));

	GPU_ShadowCaster *caster_mapping = (GPU_ShadowCaster *)frame_params->shadow_caster_table.cpu;

	if (!caster_mapping)
		return;

	for (int i = 0; i < caster_count; i++) {
		ShadowCaster *info = &frame_params->shadow_casters[i];
		GPU_ShadowCaster *gpu = &caster_mapping[i];

		// nothing past the far plane can land in the map anyway.
		float cull_radius = maths::Min(info->radius, info->far_plane);

		if (!(cull_radius > 0.f) || !std::isfinite(cull_radius))
			cull_radius = info->far_plane;

		gpu->position = info->position;
		gpu->near_plane = info->near_plane;
		gpu->far_plane = info->far_plane;
		gpu->shadow_map = device->TextureViewBindless(shadow_cubemap_views[i]);
		gpu->texel_world_scale = 2.f / (float)RENDER_SHADOW_MAP_RESOLUTION; // 90 degree face is 2 units wide at distance 1
		gpu->cull_radius = cull_radius;

		Mat4 light_proj = Mat4::Perspective(90.f, 1.f, info->far_plane, info->near_plane); // reversed-z

		for (int f = 0; f < 6; f++) {
			Vec3 target = info->position + light_dirs[f];
			Mat4 light_view = Mat4::LookAt(info->position, target, light_ups[f]);
			gpu->face_matrices[f] = light_proj * light_view;
		}

		info->content_hash = frame_params->packet ? HashCasterContents(*frame_params->packet, *info, cull_radius) : 0;
	}
}

void rnd::ShadowRenderer::Render(
	Graph *graph,
	const FrameParams *frame_params,
	Blackboard *bb
)
{
	const unsigned caster_count = maths::Min<unsigned>(frame_params->shadow_caster_count, RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS);

	bb->shadow_caster_table_addr = frame_params->shadow_caster_table.gpu;
	bb->shadow_map_count = caster_count;

	u32 render_mask = 0;

	for (int i = 0; i < caster_count; i++) {
		bb->shadow_maps[i] = graph->ImportTexture(shadow_cubemaps[i]);
		const u64 hash = frame_params->shadow_casters[i].content_hash;
		if (!cache_valid[i] || cached_hash[i] != hash)
			render_mask |= 1u << i;
	}

	if (render_mask == 0)
		return;

	DrawStream streams = CullShadowCasters(graph, frame_params, caster_count, render_mask);

	Clear depth_clear = Clear::DepthStencil(0.f, 0); // reversed-z => 0.f = far

	for (int caster_index = 0; caster_index < caster_count; caster_index++) {
		if (!(render_mask & (1u << caster_index)))
			continue;

		ShadowMappingPassData *data = graph->NewPassData<ShadowMappingPassData>();
		data->frame_params = frame_params;
		data->caster_index = caster_index;
		data->draw_stream = streams;

		char name_buf[256] = {};
		snprintf(name_buf, sizeof(name_buf), "Shadow Mapping (%u)", caster_index);
		Pass *pass = graph->Add(name_buf, PassType_Graphics);

		pass->SetRecord(ShadowMappingPassFn, data);
		pass->SetMultiViewMask(0b111111);
		pass->ReadIndirectBuffer(streams.indirect_buffer);
		pass->ReadIndirectBuffer(streams.count_buffer);
		bb->shadow_maps[caster_index] = pass->WriteDepth(bb->shadow_maps[caster_index], &depth_clear);

		cached_hash[caster_index] = frame_params->shadow_casters[caster_index].content_hash;
		cache_valid[caster_index] = true; // NOTE: IF WE DROP A GRAPH BEFORE THE GPU RUNS IT, CALL INVALIDATECACHE()!!!!
	}
}

#include "render/pass/forward_pass.h"
#include "render/graph/graph.h"
#include "graphics/device.h"

static void DepthPrepassFn(const rnd::PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const rnd::DepthPrepassData *data = static_cast<const rnd::DepthPrepassData *>(ctx.user_data);
	const rnd::FrameParams *fp = data->frame_params;
	gfx::Device *device = gfx::Device::Get();

	gfx::GraphicsPipelineDef def(fp->depth_prepass_shader, *ctx.render_info);
	def.depth_stencil_state.depth_test_enabled = true;
	def.depth_stencil_state.depth_write_enabled = true;
	def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS;

	gfx::PipelineState st = device->FetchGraphicsPipeline(def);
	cmd.BindPipelineState(st);

	struct {
		u64 frame_data;
		u64 objects;
		u64 materials;
		u64 meshes;
		u32 material_sampler;
		u32 _pad0;
	} args;

	args.frame_data = fp->frame_data.gpu;
	args.objects = fp->object_buffer.gpu;
	args.materials = fp->material_table.gpu;
	args.meshes = fp->mesh_table.gpu;
	args.material_sampler = device->SamplerBindless(fp->material_sampler);

	cmd.PushConstants(st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	gfx::ResourceKey indirect_key = ctx.graph->ResolveBuffer(data->draw_stream.indirect_buffer);
	gfx::ResourceKey counter_key = ctx.graph->ResolveBuffer(data->draw_stream.count_buffer);
	fp->RecordIndirect(cmd, indirect_key, counter_key);
}

void rnd::DepthPrepass(Graph *graph, const FrameParams *fp, Blackboard *bb, const DrawStream &stream)
{
	Pass *pass = graph->Add("Depth Prepass", PassType_Graphics);

	bb->depth_msaa = pass->WriteDepth(bb->depth_msaa, nullptr);
	pass->ReadIndirectBuffer(stream.indirect_buffer);
	pass->ReadIndirectBuffer(stream.count_buffer);

	DepthPrepassData *data = graph->NewPassData<DepthPrepassData>();
	data->frame_params = fp;
	data->draw_stream = stream;

	pass->SetRecord(DepthPrepassFn, data);
}

static void ForwardPassFn(const rnd::PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;

	const rnd::ForwardPassData *data = static_cast<const rnd::ForwardPassData *>(ctx.user_data);
	const rnd::FrameParams *frame_params = data->frame_params;

	if (data->mode == rnd::ForwardMode_Translucent && frame_params->translucent_draws.empty())
		return;

	gfx::Device *device = gfx::Device::Get();

	gfx::GraphicsPipelineDef pipeline_def(frame_params->forward_shader, *ctx.render_info);
	pipeline_def.depth_stencil_state.depth_test_enabled = true;

	if (data->mode == rnd::ForwardMode_Opaque) {
		pipeline_def.depth_stencil_state.depth_write_enabled = true;
		pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_EQUAL;
	} else {
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipeline_def.blend_state.enabled = true;
		pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
		pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_SRC_ALPHA;
		pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipeline_def.blend_state.alpha.op = VK_BLEND_OP_ADD;
		pipeline_def.blend_state.alpha.src = VK_BLEND_FACTOR_ONE;
		pipeline_def.blend_state.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}

	gfx::PipelineState pipeline_st = device->FetchGraphicsPipeline(pipeline_def);

	cmd.BindPipelineState(pipeline_st);

	struct {
		u64 frame_data_buffer;
		u64 object_buffer;
		u64 material_buffer;
		u64 mesh_buffer;

		u64 light_buffer;
		u64 shadow_caster_buffer;

		u64 irr_sh_buffer;
		u64 irr_grid_info_buffer;

		u32 irradiance_fallback_cubemap;
		u32 prefilter_cubemap;
		u32 brdf_lut;

		u32 linear_sampler;
		u32 shadow_compare_sampler;
		u32 material_sampler;

		u32 light_count;
	} args;

	args.frame_data_buffer = frame_params->frame_data.gpu;
	args.object_buffer = frame_params->object_buffer.gpu;
	args.material_buffer = frame_params->material_table.gpu;
	args.mesh_buffer = frame_params->mesh_table.gpu;

	args.light_buffer = frame_params->light_buffer.gpu;
	args.shadow_caster_buffer = data->shadow_caster_table;

	args.irr_sh_buffer = data->irradiance_sh_buffer_address;
	args.irr_grid_info_buffer = data->irradiance_grid_info_buffer_address;

	args.irradiance_fallback_cubemap = device->TextureViewBindless(ctx.graph->ResolveTextureView(data->irradiance_fb_handle, gfx::SubresourceRange::AllColour()));
	args.prefilter_cubemap = device->TextureViewBindless(ctx.graph->ResolveTextureView(data->prefilter_handle, gfx::SubresourceRange::AllColour()));
	args.brdf_lut = device->TextureViewBindless(ctx.graph->ResolveTextureView(data->brdf_handle, gfx::SubresourceRange::AllColour()));

	args.linear_sampler = device->SamplerBindless(frame_params->linear_sampler);
	args.shadow_compare_sampler = device->SamplerBindless(frame_params->shadow_compare_sampler);
	args.material_sampler = device->SamplerBindless(frame_params->material_sampler);

	args.light_count = frame_params->light_count;

	cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	if (data->mode == rnd::ForwardMode_Opaque) {
		gfx::ResourceKey indirect_key = ctx.graph->ResolveBuffer(data->opaque_draw_stream.indirect_buffer);
		gfx::ResourceKey counter_key = ctx.graph->ResolveBuffer(data->opaque_draw_stream.count_buffer);
		frame_params->RecordIndirect(cmd, indirect_key, counter_key);
	} else {
		frame_params->RecordTranslucent(cmd);
	}
}

rnd::ForwardPassData *rnd::ForwardPassDataInit(
	Graph *graph, Pass *pass,
	const FrameParams *frame_params,
	const Blackboard *bb,
	ForwardMode mode
)
{
	GraphTexHandle irradiance_fb_handle = pass->ReadTextureGraphics(graph->ImportTexture(frame_params->irradiance_fallback_cubemap));
	GraphTexHandle prefilter_handle = pass->ReadTextureGraphics(graph->ImportTexture(frame_params->prefilter_cubemap));
	GraphTexHandle brdf_handle = pass->ReadTextureGraphics(graph->ImportTexture(frame_params->brdf_lut));

	for (int i = 0; i < bb->shadow_map_count; i++)
		pass->ReadTextureGraphics(bb->shadow_maps[i]);

	ForwardPassData *data = graph->NewPassData<ForwardPassData>();
	data->frame_params = frame_params;
	data->mode = mode;
	data->shadow_caster_table = bb->shadow_caster_table_addr;
	data->irradiance_fb_handle = irradiance_fb_handle;
	data->prefilter_handle = prefilter_handle;
	data->brdf_handle = brdf_handle;

	// todo: disabled for now
	data->irradiance_sh_buffer_address = 0;
	data->irradiance_grid_info_buffer_address = 0;

	return data;
}

void rnd::ForwardRender(
	Graph *graph,
	const FrameParams *frame_params,
	Blackboard *bb,
	const DrawStream &opaque_draw_stream
)
{
	Pass *pass = graph->Add("Forward Opaque", PassType_Graphics);

	bb->lighting_msaa = pass->WriteColour(bb->lighting_msaa, nullptr);
	bb->depth_msaa = pass->WriteDepth(bb->depth_msaa, nullptr);

	pass->ReadIndirectBuffer(opaque_draw_stream.indirect_buffer);
	pass->ReadIndirectBuffer(opaque_draw_stream.count_buffer);

	ForwardPassData *data = ForwardPassDataInit(graph, pass, frame_params, bb, ForwardMode_Opaque);
	data->opaque_draw_stream = opaque_draw_stream;

	pass->SetRecord(ForwardPassFn, data);
}

void rnd::ForwardRenderTranslucent(
	Graph *graph,
	const FrameParams *frame_params,
	Blackboard *bb
)
{
	Pass *pass = graph->Add("Forward Translucent", PassType_Graphics);

	bb->lighting_resolve = pass->WriteColourResolve(bb->lighting_msaa, bb->lighting_resolve, nullptr);
	bb->depth_resolve = pass->WriteDepthResolve(bb->depth_msaa, bb->depth_resolve, nullptr);

	ForwardPassData *data = ForwardPassDataInit(graph, pass, frame_params, bb, ForwardMode_Translucent);

	pass->SetRecord(ForwardPassFn, data);
}

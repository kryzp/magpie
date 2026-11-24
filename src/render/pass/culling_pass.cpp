#include "render/pass/culling_pass.h"
#include "render/graph/graph.h"
#include "graphics/device.h"
#include "graphics/util.h"

void rnd::CullClearFn(const PassContext &ctx)
{
	const CullClearPassData *data = (const CullClearPassData *)ctx.user_data;

	gfx::ResourceKey counter_key = ctx.graph->ResolveBuffer(data->counter_handle);
	ctx.cmd->FillBuffer(counter_key, 0, gfx::Device::Get()->BufferSize(counter_key), 0);
}

static void CullFrustumComputeFn(const rnd::PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;

	const rnd::CullPassData *data = (const rnd::CullPassData *)ctx.user_data;

	gfx::ComputePipelineDef pipeline_def(data->shader);
	gfx::PipelineState pipeline_st = gfx::Device::Get()->FetchComputePipeline(pipeline_def);

	rnd::BufferRange indirect_range = ctx.graph->ResolveBufferRange(data->indirect_handle);
	rnd::BufferRange counter_range = ctx.graph->ResolveBufferRange(data->counter_handle);

	struct {
		u64 object_buffer;
		u64 mesh_buffer;
		u64 material_buffer;
		u64 page_buffer;
		u64 indirect_buffer;
		u64 count_buffer;
		u32 object_count;
		u32 alpha_filter;
		u32 max_draws_per_page;
		u32 _padding1;
		Vec4 frustum_planes[6];
	} pc;

	pc.object_buffer = data->frame_params->object_buffer.gpu;
	pc.mesh_buffer = data->frame_params->mesh_table.gpu;
	pc.material_buffer = data->frame_params->material_table.gpu;
	pc.page_buffer = data->frame_params->page_table_buffer.gpu;
	pc.indirect_buffer = indirect_range.Address();
	pc.count_buffer = counter_range.Address();
	pc.object_count = data->frame_params->object_count;
	pc.alpha_filter = data->filter;
	pc.max_draws_per_page = RENDER_SCENE_MAX_ENTITIES;

	for (int i = 0; i < 6; i++)
		pc.frustum_planes[i] = data->frustum_planes[i];

	cmd.BindPipelineState(pipeline_st);
	cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, pc, 0);
	cmd.Dispatch(gfx::ComputeGroupCount(pc.object_count, 64), 1, 1);
}

static void CullShadowCastersComputeFn(const rnd::PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;

	const rnd::ShadowCullPassData *data = (const rnd::ShadowCullPassData *)ctx.user_data;

	gfx::ComputePipelineDef pipeline_def(data->shader);
	gfx::PipelineState pipeline_st = gfx::Device::Get()->FetchComputePipeline(pipeline_def);

	rnd::BufferRange indirect_range = ctx.graph->ResolveBufferRange(data->indirect_handle);
	rnd::BufferRange counter_range = ctx.graph->ResolveBufferRange(data->counter_handle);

	struct {
		u64 object_buffer;
		u64 mesh_buffer;
		u64 material_buffer;
		u64 caster_buffer;
		u64 indirect_buffer;
		u64 count_buffer;
		u32 object_count;
		u32 caster_mask;
		u32 max_draws_per_page;
		u32 page_count;
	} pc;

	pc.object_buffer = data->frame_params->object_buffer.gpu;
	pc.mesh_buffer = data->frame_params->mesh_table.gpu;
	pc.material_buffer = data->frame_params->material_table.gpu;
	pc.caster_buffer = data->frame_params->shadow_caster_table.gpu;
	pc.indirect_buffer = indirect_range.Address();
	pc.count_buffer = counter_range.Address();
	pc.object_count = data->frame_params->object_count;
	pc.caster_mask = data->caster_mask;
	pc.max_draws_per_page = RENDER_SCENE_MAX_ENTITIES;
	pc.page_count = data->frame_params->page_count;

	cmd.BindPipelineState(pipeline_st);
	cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, pc, 0);

	cmd.Dispatch(gfx::ComputeGroupCount(pc.object_count, 64), data->caster_count, 1);
}

static void CreateDrawStreamBuffers(
	rnd::Graph *graph, const rnd::FrameParams *frame_params,
	rnd::GraphBufHandle *out_indirect, rnd::GraphBufHandle *out_counter,
	unsigned stream_count = 1
)
{
	rnd::BufferInfo indirect_info(
		RENDER_SCENE_MAX_ENTITIES * sizeof(rnd::GPU_IndirectDraw) * frame_params->page_count * stream_count,
		0,
		VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT
	);

	rnd::BufferInfo counter_info(
		sizeof(unsigned) * frame_params->page_count * stream_count,
		0,
		VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT
	);

	*out_indirect = graph->CreateBuffer(indirect_info);
	*out_counter = graph->CreateBuffer(counter_info);
}

rnd::DrawStream rnd::CullFrustum(
	Graph *graph,
	const FrameParams *frame_params,
	CullFilter filter,
	const FrustumVolume &frustum
)
{
	DrawStream stream;

	GraphBufHandle indirect_handle;
	GraphBufHandle counter_handle;

	CreateDrawStreamBuffers(graph, frame_params, &indirect_handle, &counter_handle);

	// Clear counter pass.
	{
		CullClearPassData *data = graph->NewPassData<CullClearPassData>();
		data->counter_handle = counter_handle;

		Pass *pass = graph->Add("Compute Frustum Culling (Clear Counter)", PassType_Transfer);
		pass->SetRecord(CullClearFn, data);
		pass->ClearBuffer(counter_handle);
	}

	// Compute culling pass.
	{
		CullPassData *data = graph->NewPassData<CullPassData>();
		data->shader = frame_params->cull_frustum_shader;
		data->indirect_handle = indirect_handle;
		data->counter_handle = counter_handle;
		data->frame_params = frame_params;
		data->filter = filter;

		for (int i = 0; i < 6; i++)
			data->frustum_planes[i] = frustum.planes[i];

		Pass *pass = graph->Add("Compute Frustum Culling", PassType_Compute);
		pass->SetRecord(CullFrustumComputeFn, data);

		stream.indirect_buffer = pass->WriteBufferCompute(indirect_handle);
		stream.count_buffer = pass->WriteBufferCompute(counter_handle);
	}

	return stream;
}

rnd::DrawStream rnd::CullShadowCasters(
	Graph *graph,
	const FrameParams *frame_params,
	unsigned caster_count,
	u32 caster_mask
)
{
	DrawStream stream;

	GraphBufHandle indirect_handle;
	GraphBufHandle counter_handle;

	CreateDrawStreamBuffers(graph, frame_params, &indirect_handle, &counter_handle, caster_count * ShadowStream_Count);

	// Clear counter pass.
	{
		CullClearPassData *data = graph->NewPassData<CullClearPassData>();
		data->counter_handle = counter_handle;

		Pass *pass = graph->Add("Compute Shadow Culling (Clear Counters)", PassType_Transfer);
		pass->SetRecord(CullClearFn, data);
		pass->ClearBuffer(counter_handle);
	}

	// Dispatches for every caster that needs re-rendering in one compute pass.
	{
		ShadowCullPassData *data = graph->NewPassData<ShadowCullPassData>();
		data->shader = frame_params->shadow_cull_shader;
		data->indirect_handle = indirect_handle;
		data->counter_handle = counter_handle;
		data->frame_params = frame_params;
		data->caster_count = caster_count;
		data->caster_mask = caster_mask;

		Pass *pass = graph->Add("Compute Shadow Culling", PassType_Compute);
		pass->SetRecord(CullShadowCastersComputeFn, data);

		stream.indirect_buffer = pass->WriteBufferCompute(indirect_handle);
		stream.count_buffer = pass->WriteBufferCompute(counter_handle);
	}

	return stream;
}

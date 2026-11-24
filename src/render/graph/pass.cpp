#include "render/graph/pass.h"
#include "render/graph/graph.h"
#include "core/log.h"

rnd::Pass::Pass()
	: graph(nullptr)
	, name("")
	, type(PassType_Graphics)
	, index(0)
	, is_culled(false)
	, record(nullptr)
	, user_data(nullptr)
	, multi_view_mask(0)
	, input_textures()
	, output_textures()
	, input_buffers()
	, output_buffers()
	, memory_barriers()
	, texture_barriers()
	, buffer_barriers()
{
}

void rnd::Pass::SetRecord(PassRecordFn *fn, const void *user_data_)
{
	record = fn;
	user_data = user_data_;
}

void rnd::Pass::SetMultiViewMask(unsigned mask)
{
	multi_view_mask = mask;
}

rnd::GraphTexHandle rnd::Pass::AddInputTexture(
	GraphTexHandle handle,
	VkPipelineStageFlags2 stage,
	VkAccessFlags2 access
)
{
	GraphTexHandle versioned_handle = handle;

	PassTextureEdge edge;
	edge.handle = versioned_handle;
	edge.state.stage = stage;
	edge.state.access = access;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.should_clear = false;

	input_textures.push_back(edge);

	return versioned_handle;
}

rnd::GraphTexHandle rnd::Pass::AddOutputTexture(
	GraphTexHandle handle,
	const Clear *clear,
	VkPipelineStageFlags2 stage,
	VkAccessFlags2 access
)
{
	GraphTexHandle versioned_handle = graph->PushTexVersion(handle, index);

	PassTextureEdge edge;
	edge.handle = versioned_handle;
	edge.state.stage = stage;
	edge.state.access = access;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.should_clear = clear != nullptr;

	if (edge.should_clear)
		edge.clear = *clear;

	output_textures.push_back(edge);

	return versioned_handle;
}

rnd::GraphBufHandle rnd::Pass::AddInputBuffer(
	GraphBufHandle handle,
	VkPipelineStageFlags2 stage,
	VkAccessFlags2 access
)
{
	GraphBufHandle versioned_handle = handle;

	PassBufferEdge edge;
	edge.handle = versioned_handle;
	edge.state.stage = stage;
	edge.state.access = access;
	edge.offset = 0;
	edge.size = 0;

	input_buffers.push_back(edge);

	return versioned_handle;
}

rnd::GraphBufHandle rnd::Pass::AddOutputBuffer(
	GraphBufHandle handle,
	VkPipelineStageFlags2 stage,
	VkAccessFlags2 access
)
{
	GraphBufHandle versioned_handle = graph->PushBufVersion(handle, index);

	PassBufferEdge edge;
	edge.handle = versioned_handle;
	edge.state.stage = stage;
	edge.state.access = access;
	edge.offset = 0;
	edge.size = 0;

	output_buffers.push_back(edge);

	return versioned_handle;
}

rnd::GraphTexHandle rnd::Pass::WriteColour(GraphTexHandle handle, const Clear *clear)
{
	return WriteColourEx(handle, clear, gfx::SubresourceRange::AllColour());
}

rnd::GraphTexHandle rnd::Pass::WriteDepth(GraphTexHandle handle, const Clear *clear)
{
	return WriteDepthEx(handle, clear, gfx::SubresourceRange::AllDepth());
}

rnd::GraphTexHandle rnd::Pass::WriteColourEx(GraphTexHandle handle, const Clear *clear, gfx::SubresourceRange range)
{
	GraphTexHandle versioned_handle = graph->PushTexVersion(handle, index);

	PassTextureEdge edge;
	edge.handle = versioned_handle;
	edge.state.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	edge.state.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.attachment_range = range;
	edge.should_clear = clear != nullptr;

	if (edge.should_clear)
		edge.clear = *clear;

	output_textures.push_back(edge);

	return versioned_handle;
}

rnd::GraphTexHandle rnd::Pass::WriteDepthEx(GraphTexHandle handle, const Clear *clear, gfx::SubresourceRange range)
{
	GraphTexHandle versioned_handle = graph->PushTexVersion(handle, index);

	PassTextureEdge edge;
	edge.handle = versioned_handle;
	edge.state.stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	edge.state.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.attachment_range = range;
	edge.should_clear = clear != nullptr;

	if (edge.should_clear)
		edge.clear = *clear;

	output_textures.push_back(edge);

	return versioned_handle;
}

rnd::GraphTexHandle rnd::Pass::WriteColourResolve(GraphTexHandle msaa, GraphTexHandle resolve, const Clear *clear)
{
	return WriteColourResolveEx(msaa, resolve, clear, gfx::SubresourceRange::AllColour());
}

rnd::GraphTexHandle rnd::Pass::WriteDepthResolve(GraphTexHandle msaa, GraphTexHandle resolve, const Clear *clear)
{
	return WriteDepthResolveEx(msaa, resolve, clear, gfx::SubresourceRange::AllDepth());
}

rnd::GraphTexHandle rnd::Pass::WriteColourResolveEx(GraphTexHandle msaa, GraphTexHandle resolve, const Clear *clear, gfx::SubresourceRange range)
{
	GraphTexHandle msaa_v = graph->PushTexVersion(msaa, index);
	GraphTexHandle resolve_v = graph->PushTexVersion(resolve, index);

	PassTextureEdge edge;
	edge.handle = msaa_v;
	edge.state.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	edge.state.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.attachment_range = range;
	edge.resolve_handle = resolve_v;
	edge.resolve_mode = VK_RESOLVE_MODE_AVERAGE_BIT;
	edge.resolve_layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.should_clear = clear != nullptr;

	if (edge.should_clear)
		edge.clear = *clear;

	output_textures.push_back(edge);

	return resolve_v; // downstream passes get the resolved version
}

rnd::GraphTexHandle rnd::Pass::WriteDepthResolveEx(GraphTexHandle msaa, GraphTexHandle resolve, const Clear *clear, gfx::SubresourceRange range)
{
	GraphTexHandle msaa_v = graph->PushTexVersion(msaa, index);
	GraphTexHandle resolve_v = graph->PushTexVersion(resolve, index);

	PassTextureEdge edge;
	edge.handle = msaa_v;
	edge.state.stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	edge.state.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.attachment_range = range;
	edge.resolve_handle = resolve_v;
	edge.resolve_mode = VK_RESOLVE_MODE_MIN_BIT;
	edge.resolve_layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.should_clear = clear != nullptr;

	if (edge.should_clear)
		edge.clear = *clear;

	output_textures.push_back(edge);

	return resolve_v; // downstream passes get the resolved version
}

rnd::GraphTexHandle rnd::Pass::ReadTextureGraphics(GraphTexHandle handle)
{
	return AddInputTexture(
		handle,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_ACCESS_2_SHADER_READ_BIT
	);
}

rnd::GraphTexHandle rnd::Pass::ReadTextureCompute(GraphTexHandle handle)
{
	return AddInputTexture(
		handle,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		VK_ACCESS_2_SHADER_READ_BIT
	);
}

rnd::GraphTexHandle rnd::Pass::WriteTextureCompute(GraphTexHandle handle)
{
	return AddOutputTexture(
		handle, nullptr,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		VK_ACCESS_2_SHADER_WRITE_BIT
	);
}

rnd::GraphTexHandle rnd::Pass::BlitTextureSrc(GraphTexHandle handle)
{
	return AddInputTexture(
		handle,
		VK_PIPELINE_STAGE_2_BLIT_BIT,
		VK_ACCESS_2_TRANSFER_READ_BIT
	);
}

rnd::GraphTexHandle rnd::Pass::BlitTextureDst(GraphTexHandle handle)
{
	return AddOutputTexture(
		handle, nullptr,
		VK_PIPELINE_STAGE_2_BLIT_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT
	);
}

rnd::GraphBufHandle rnd::Pass::WriteBufferGraphics(GraphBufHandle handle)
{
	return AddOutputBuffer(
		handle,
		VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_ACCESS_2_SHADER_WRITE_BIT
	);
}

rnd::GraphBufHandle rnd::Pass::ReadBufferGraphics(GraphBufHandle handle)
{
	return AddInputBuffer(
		handle,
		VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		VK_ACCESS_2_SHADER_READ_BIT
	);
}

rnd::GraphBufHandle rnd::Pass::WriteBufferCompute(GraphBufHandle handle)
{
	return AddOutputBuffer(
		handle,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		VK_ACCESS_2_SHADER_WRITE_BIT
	);
}

rnd::GraphBufHandle rnd::Pass::ReadBufferCompute(GraphBufHandle handle)
{
	return AddInputBuffer(
		handle,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		VK_ACCESS_2_SHADER_READ_BIT
	);
}

rnd::GraphBufHandle rnd::Pass::ReadIndirectBuffer(GraphBufHandle handle)
{
	return AddInputBuffer(
		handle,
		VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
		VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT
	);
}

rnd::GraphBufHandle rnd::Pass::ClearBuffer(GraphBufHandle handle)
{
	return AddOutputBuffer(
		handle,
		VK_PIPELINE_STAGE_2_CLEAR_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT
	);
}

#include "render_graph.h"

#include "math/calc.h"

#include "gpu_profiler.h"

using namespace gfx;

RenderGraphBlackboard::RenderGraphBlackboard()
	: items{}
{
}

RenderGraphBlackboard::~RenderGraphBlackboard()
{
}

void RenderGraphBlackboard::clean()
{
	items.clear();
}

RenderStageResources::RenderStageResources(RenderGraph &graph, const RenderStage &stage)
	: graph(graph)
	, stage(stage)
{
}

RenderStageResources::~RenderStageResources()
{
}

RenderInfo RenderStageResources::build_rendering_info() const
{
	RenderInfo render_info = {};

	render_info.view_mask = stage.get_multi_view_mask();

	for (auto &output : stage.get_outputs()) {
		auto &attachment_info = graph.resources[output.handle].texture_info;

		// TODO: Right now it's just based on the last attachments sample count_offset.
		//       Assumption is that all attachments will already have the same sample count_offset.
		//       --> Ideally I should have resolving implemented so they automatically have their resolves.
		render_info.samples = attachment_info.samples;

		u32 mip = output.range.base_mip;
		render_info.width = CalcU::max(1u, (u32)attachment_info.size_x >> mip);
		render_info.height = CalcU::max(1u, (u32)attachment_info.size_y >> mip);

		VkRenderingAttachmentInfo vk_attachment_info = {};
		vk_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

		vk_attachment_info.loadOp = output.clear_enabled ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		vk_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		vk_attachment_info.imageView = get_texture_view(output.handle, output.range)->get_handle();
		vk_attachment_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		
		// TODO: MSAA isn't supported yet.
		vk_attachment_info.resolveImageView = VK_NULL_HANDLE;
		vk_attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vk_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;

		const RenderClear &clear = output.clear;

		if (attachment_info.format == graph.get_device().get_context().get_depth_format()) {
			vk_attachment_info.clearValue.depthStencil = {
				clear.depth,
				clear.stencil
			};

			render_info.depth_attachment = vk_attachment_info;
		} else {
			vk_attachment_info.clearValue.color = {
				clear.colour.r,
				clear.colour.g,
				clear.colour.b,
				clear.colour.a
			};

			render_info.colour_attachments.push_back(vk_attachment_info);
		}
	}

	return render_info;
}

const Texture *RenderStageResources::get_texture(RenderResourceHandle handle) const
{
	return graph.resources[handle].physical_texture;
}

const TextureView *RenderStageResources::get_texture_view(RenderResourceHandle handle, const SubresourceRange &range) const
{
	const Texture *physical_texture = graph.resources[handle].physical_texture;

	return graph.cache->fetch_texture_view(
		physical_texture,
		physical_texture->get_default_view_type(),
		range
	);
}

const GpuBuffer *RenderStageResources::get_buffer(RenderResourceHandle handle) const
{
	return graph.resources[handle].physical_buffer;
}

GpuBufferRange RenderStageResources::get_buffer_range(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.resources[handle];
	const GpuBuffer *physical_buffer = graph.resources[handle].physical_buffer;
	return GpuBufferRange(physical_buffer, resource.buffer_info.size, resource.buffer_offset);
}

RenderStage::RenderStage()
	: name()
	, type(TYPE_MAX_ENUM)
	, index(-1u)
	, record()
	, multi_view_mask(0)
	, inputs()
	, outputs()
	, memory_barriers()
	, buffer_barriers()
	, texture_barriers()
	, is_culled(false)
{
}

RenderStage::~RenderStage()
{
}

void RenderStage::set_record(std::function<void(const RenderContext &ctx, const RenderStageResources &resources)> fn)
{
	record = fn;
}

void RenderStage::set_multi_view_mask(u32 mask)
{
	multi_view_mask = mask;
}

u32 RenderStage::get_multi_view_mask() const
{
	return multi_view_mask;
}

const Vector<RenderResourceEdge> &RenderStage::get_inputs() const
{
	return inputs;
}

const Vector<RenderResourceEdge> &RenderStage::get_outputs() const
{
	return outputs;
}

RenderResourceHandle RenderStage::add_edge(
	RenderResourceHandle handle,
	const AccessState &state,
	const SubresourceRange &range,
	bool is_output, const RenderClear *clear
)
{
	RenderResourceEdge edge = {};
	edge.handle = handle;
	edge.access_state = state;
	edge.range = range;
	
	if (clear) {
		edge.clear_enabled = true;
		edge.clear = *clear;
	} else {
		edge.clear_enabled = false;
	}

	if (is_output) {
		outputs.push_back(edge);
	} else {
		inputs.push_back(edge);
	}

	return handle;
}

RenderResourceHandle RenderStage::write_colour(RenderResourceHandle handle, const SubresourceRange &range, const RenderClear *clear)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
		},
		range, true, clear
	);
}

RenderResourceHandle RenderStage::write_depth(RenderResourceHandle handle, const SubresourceRange &range, const RenderClear *clear)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
		},
		range, true, clear
	);
}

RenderResourceHandle RenderStage::read_texture(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT
		},
		{}, false, nullptr
	);
}

RenderResourceHandle RenderStage::read_texture_compute(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT
		},
		{}, false, nullptr
	);
}

RenderResourceHandle RenderStage::write_texture_compute(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT
		},
		{}, true, nullptr
	);
}

RenderResourceHandle RenderStage::blit_texture_src(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_BLIT_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT
		},
		{}, false, nullptr
	);
}

RenderResourceHandle RenderStage::blit_texture_dst(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_BLIT_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT
		},
		{}, true, nullptr
	);
}

RenderResourceHandle RenderStage::write_buffer_graphics(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT
		},
		{}, true, nullptr
	);
}

RenderResourceHandle RenderStage::read_buffer_graphics(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT
		},
		{}, false, nullptr
	);
}

RenderResourceHandle RenderStage::write_buffer_compute(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT
		},
		{}, true, nullptr
	);
}

RenderResourceHandle RenderStage::read_buffer_compute(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT
		},
		{}, false, nullptr
	);
}

RenderResourceHandle RenderStage::indirect_buffer(RenderResourceHandle handle)
{
	return add_edge(
		handle, 
		{
			VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
			VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT
		},
		{}, false, nullptr
	);
}

RenderResourceHandle RenderStage::clear_buffer(RenderResourceHandle handle)
{
	return add_edge(
		handle,
		{
			VK_PIPELINE_STAGE_2_CLEAR_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT
		},
		{}, true, nullptr
	);
}

RenderResourcePool::RenderResourcePool(RenderGraph &graph)
	: graph(graph)
	, current_time(0)
	, gpu_completed_time(0)
	, texture_pool()
	, buffer_pool()
{
}

RenderResourcePool::~RenderResourcePool()
{
}

void RenderResourcePool::destroy()
{
	for (auto &t : texture_pool) {
		assert(t.in_use == false);
		graph.get_device().destroy_texture(t.texture);
		t.texture = nullptr;
	}

	for (auto &b : buffer_pool) {
		assert(b.in_use == false);
		graph.get_device().destroy_buffer(b.buffer);
		b.buffer = nullptr;
	}

	texture_pool.clear();
	buffer_pool.clear();
}

void RenderResourcePool::flush()
{
	current_time = graph.get_device().get_graphics_timeline_value() + 1;
	gpu_completed_time = graph.get_device().get_graphics_completed_timeline_value();

	for (auto &t : texture_pool)
		t.in_use = false;

	for (auto &b : buffer_pool)
		b.in_use = false;

	/*
	const u64 GARBAGE_COLLECT_THRESHOLD = 120;

	for (auto t = texture_pool.begin(); t != texture_pool.end();) {
		t->in_use = false;

		if (current_frame - t->last_frame_used >= GARBAGE_COLLECT_THRESHOLD) {
			graph.get_device().destroy_texture(t->texture);
			t->texture = nullptr;
			t = texture_pool.erase(t);
		} else {
			t++;
		}
	}

	for (auto b = buffer_pool.begin(); b != buffer_pool.end();) {
		b->in_use = false;

		if (current_frame - b->last_frame_used >= GARBAGE_COLLECT_THRESHOLD) {
			graph.get_device().destroy_buffer(b->buffer);
			b->buffer = nullptr;
			b = buffer_pool.erase(b);
		} else {
			b++;
		}
	}
	*/
}

const Texture *RenderResourcePool::acquire_texture(const AttachmentInfo &info, ResourceTrackingState *out_state)
{
	for (auto &t : texture_pool) {
		bool gpu_done = t.last_frame_used <= gpu_completed_time;
		if (!t.in_use && gpu_done && t.info == info) {
			t.in_use = true;
			t.last_frame_used = current_time;

			if (out_state)
				*out_state = t.state;

			return t.texture;
		}
	}
	
	PooledTexture texture = {};

	texture.texture = graph.get_device().alloc_texture(
		info.size_x,
		info.size_y,
		info.size_z,
		info.format,
		info.size_z > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		info.mips,
		info.layers,
		info.samples,
		info.is_transient,
		info.is_storage,
		info.is_cubemap
	);

	texture.info = info;

	texture.in_use = true;
	texture.last_frame_used = current_time;

	texture.state.pipeline_barrier_stage_flags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	texture.state.to_flush_access = VK_ACCESS_2_NONE;
	texture.state.layout = VK_IMAGE_LAYOUT_UNDEFINED;

	memory_zero_array(texture.state.invalidated_in_stage);
	
	if (out_state)
		*out_state = texture.state;

	texture_pool.push_back(texture);

	return texture.texture;
}

/*
void RenderResourcePool::release_texture(const Texture *texture, const AttachmentInfo &info)
{
	PooledTexture resource = {};
	resource.texture = texture;
	resource.texture_info = info;

	texture_pool.push_back(resource);
}
*/

const GpuBuffer *RenderResourcePool::acquire_buffer(const GpuBufferInfo &info, ResourceTrackingState *out_state)
{
	for (auto &b : buffer_pool) {
		bool gpu_done = b.last_frame_used <= gpu_completed_time;
		if (!b.in_use && gpu_done && b.info == info) {
			b.in_use = true;
			b.last_frame_used = current_time;

			if (out_state)
				*out_state = b.state;

			return b.buffer;
		}
	}
	
	PooledBuffer buffer = {};

	buffer.buffer = graph.get_device().alloc_buffer(
		info.usage,
		info.flags,
		info.size
	);

	buffer.info = info;

	buffer.in_use = true;
	buffer.last_frame_used = current_time;

	buffer.state.pipeline_barrier_stage_flags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	buffer.state.to_flush_access = VK_ACCESS_2_NONE;
	
	memory_zero_array(buffer.state.invalidated_in_stage);
	
	if (out_state)
		*out_state = buffer.state;

	buffer_pool.push_back(buffer);

	return buffer.buffer;
}

void RenderResourcePool::update_texture_state(const Texture *texture, const ResourceTrackingState &state)
{
	for (auto &t : texture_pool) {
		if (t.texture == texture) {
			t.state = state;
			return;
		}
	}
}

void RenderResourcePool::update_buffer_state(const GpuBuffer *buffer, const ResourceTrackingState &state)
{
	for (auto &b : buffer_pool) {
		if (b.buffer == buffer) {
			b.state = state;
			return;
		}
	}
}

/*
void RenderResourcePool::release_buffer(const GpuBuffer *buffer, const GpuBufferInfo &info)
{
	PooledBuffer resource = {};
	resource.buffer = buffer;
	resource.buffer_info = info;

	buffer_pool.push_back(resource);
}
*/

static bool resource_needs_invalidation(const AccessState &barrier, const ResourceTrackingState &tracking)
{
	const u64 stages = barrier.stage;

	for (int b = 0; b < array_size(tracking.invalidated_in_stage) && stages != 0; b++) {
		if ((stages >> b) & 1) {
			if (barrier.access & ~tracking.invalidated_in_stage[b])
				return true;
		}
	}

	return false;
}

RenderGraph::RenderGraph()
	: device(nullptr)
	, cache(nullptr)
	, resources()
	, stages()
	, pool(*this)
	, import_cache()
	, backbuffer_handle(RENDER_INVALID_HANDLE)
	, tracked_external_textures()
	, tracked_external_buffers()
{
}

RenderGraph::~RenderGraph()
{
}

void RenderGraph::init(Device *device, ResourceCache *cache)
{
	assert(device && cache);

	this->device = device;
	this->cache = cache;
}

void RenderGraph::destroy()
{
	pool.destroy();
	resources.clear();
	stages.clear();
	import_cache.clear();

	backbuffer_handle = RENDER_INVALID_HANDLE;

	tracked_external_textures.clear();
	tracked_external_buffers.clear();
}

void RenderGraph::reset()
{
	for (auto &r : resources) {
		if (r.is_imported) {
			switch (r.kind) {
				case RenderResource::KIND_TEXTURE:
					tracked_external_textures[r.physical_texture] = r.tracking;
					break;

				case RenderResource::KIND_BUFFER:
					tracked_external_buffers[r.physical_buffer] = r.tracking;
					break;
			}
		} else {
			switch (r.kind) {
				case RenderResource::KIND_TEXTURE:
					pool.update_texture_state(r.physical_texture, r.tracking);
					break;

				case RenderResource::KIND_BUFFER:
					pool.update_buffer_state(r.physical_buffer, r.tracking);
					break;
			}
		}
	}

	stages.clear();
	resources.clear();
	import_cache.clear();
	
	pool.flush();

	backbuffer_handle = RENDER_INVALID_HANDLE;
}

RenderStage &RenderGraph::push_stage(const String &name, RenderStage::Type type)
{
	RenderStage stage = {};
	stage.name = name;
	stage.type = type;
	stage.index = stages.size();

	stages.push_back(stage);

	return stages.back();
}

void RenderGraph::set_backbuffer_source(RenderResourceHandle handle)
{
	backbuffer_handle = handle;
}

void RenderGraph::compile(const Swapchain &swapchain)
{
	for (auto &r : resources) {
		if (r.is_imported)
			r.ref_count = 1;
	}

	if (backbuffer_handle != RENDER_INVALID_HANDLE)
		resources[backbuffer_handle].ref_count++;

	propogate_dependencies();
	backpropogate_dependencies();

	allocate_resources(swapchain);

	generate_barriers();
}

void RenderGraph::propogate_dependencies()
{
	for (int i = 0; i < stages.size(); i++) {
		RenderStage &stage = stages[i];
		
		for (auto &out : stage.outputs) {
			if (resources[out.handle].first_stage_index == -1u)
				resources[out.handle].first_stage_index = i;
		}

		if (!stage.is_culled) {
			for (auto &in : stage.inputs) {
				if (resources[in.handle].first_stage_index == -1u)
					resources[in.handle].first_stage_index = i;
			}
		}
	}
}

void RenderGraph::backpropogate_dependencies()
{
	for (int i = stages.size() - 1; i >= 0; i--) {
		RenderStage &stage = stages[i];

		stage.is_culled = false;

		for (auto &out : stage.outputs) {
			if (out.handle == backbuffer_handle)
				stage.is_culled = false;

			if (resources[out.handle].ref_count > 0)
				stage.is_culled = false;

			if (resources[out.handle].last_stage_index == -1u)
				resources[out.handle].last_stage_index = i;
		}

		if (!stage.is_culled) {
			for (auto &in : stage.inputs) {
				resources[in.handle].ref_count++;

				if (resources[in.handle].last_stage_index == -1u)
					resources[in.handle].last_stage_index = i;
			}
		}
	}
}

void RenderGraph::allocate_resources(const Swapchain &swapchain)
{
	for (auto &stage : stages) {
		if (stage.is_culled)
			continue;

		for (auto &out : stage.outputs) {
			RenderResource &r = resources[out.handle];

			if (r.is_imported)
				continue;

			if (r.physical_texture || r.physical_buffer)
				continue;

			switch (r.kind) {
				case RenderResource::KIND_TEXTURE:
					// Resolve rleative size.
					if (r.texture_info.size_class == SIZE_CLASS_SWAPCHAIN_RELATIVE) {
						r.texture_info.size_x *= swapchain.get_width();
						r.texture_info.size_y *= swapchain.get_height();
						r.texture_info.size_class = SIZE_CLASS_ABSOLUTE;
					}

					r.physical_texture = pool.acquire_texture(r.texture_info, &r.tracking);
					break;

				case RenderResource::KIND_BUFFER:
					r.physical_buffer = pool.acquire_buffer(r.buffer_info, &r.tracking);
					break;
			}
		}
	}
}

void RenderGraph::generate_barriers()
{
	for (auto &stage : stages) {
		if (stage.is_culled)
			continue;
		
		for (auto &in : stage.inputs)
			process_invalidate(stage, in);

		for (auto &out : stage.outputs)
			process_invalidate(stage, out);

		for (auto &out : stage.outputs)
			process_flush(stage, out);
	}
}

void RenderGraph::process_invalidate(RenderStage &stage, const RenderResourceEdge &edge)
{
	RenderResource &r = resources[edge.handle];
	
	if (!r.physical_texture && !r.physical_buffer)
		return;

	const VkImageLayout target_layout = VK_IMAGE_LAYOUT_GENERAL;

	bool layout_change = (r.kind == RenderResource::KIND_TEXTURE) && (r.tracking.layout != target_layout);

	bool needs_sync = layout_change || (r.tracking.to_flush_access != 0) || resource_needs_invalidation(edge.access_state, r.tracking);

	if (needs_sync) {
		VkPipelineStageFlags2 src_stage = r.tracking.pipeline_barrier_stage_flags ? r.tracking.pipeline_barrier_stage_flags : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		VkAccessFlags2 src_access = r.tracking.to_flush_access;

		if (r.kind == RenderResource::KIND_TEXTURE) {
			stage.texture_barriers.push_back(sync::texture_memory_barrier(
				r.physical_texture,
				{ src_stage, src_access },
				edge.access_state,
				r.tracking.layout, target_layout,
				0, VK_REMAINING_MIP_LEVELS,
				0, VK_REMAINING_ARRAY_LAYERS
			));

			r.tracking.layout = target_layout;
		} else if (r.kind == RenderResource::KIND_BUFFER) {
			stage.buffer_barriers.push_back(sync::buffer_memory_barrier(
				r.physical_buffer,
				{ src_stage, src_access },
				edge.access_state,
				r.buffer_offset, r.buffer_info.size
			));
		}

		if (r.tracking.to_flush_access || layout_change)
			memory_zero_array(r.tracking.invalidated_in_stage);

		r.tracking.to_flush_access = 0;

		if (layout_change) {
			const u64 dst_stages = edge.access_state.stage;

			for (int i = 0; i < array_size(r.tracking.invalidated_in_stage); i++) {
				if ((dst_stages >> i) & 1)
					r.tracking.invalidated_in_stage[i] |= edge.access_state.access;
			}
		}
	}

	r.tracking.pipeline_barrier_stage_flags = edge.access_state.stage;
}

void RenderGraph::process_flush(RenderStage &stage, const RenderResourceEdge &edge)
{
	RenderResource &r = resources[edge.handle];
	
	if (!r.physical_texture && !r.physical_buffer)
		return;

	r.tracking.to_flush_access = edge.access_state.access;
	r.tracking.pipeline_barrier_stage_flags = edge.access_state.stage;
}

#if 0
	/*
	 * Right now we just use a run-length encoding style algorithm,
	 * but could this be optimized with a flood-fill style algorithm?
	 * 
	 * Must be only squares though.
	 *
	 * Kinda like this (MIP x LAYER, { A, B, C, ... } are access states):
	 *
	 *          0   1   2   3
	 *        +---+----------
	 *      0 | A | B   B   B
	 *        +---+
	 *      1 | C | B   B   B   ...
	 *        |   +-----------
	 *      2 | C | E | F   F
	 *        +---+   |
	 *      3 | D | E | F   F
	 *              ...
	 *
	 * Then each "block" gets a pipeline barrier.
	 */

	const Texture *physical_texture = (const Texture *)t.physical_resource;

	const u32 total_mips = t.texture_info.mips;
	const int base_mip = range.base_mip;
	const int base_layer = range.base_layer;

	SubresourceRange texture_range = range.of_texture(physical_texture);

	auto subresource_idx = [&](u32 mip, u32 layer) -> u32 {
		return (layer * total_mips) + mip;
	};

	auto push_barrier = [&](TextureAccessType src, u32 base_mip, u32 mips, u32 base_layer, u32 layers) -> void {
		barriers.push_back(sync::texture_memory_barrier(
			physical_texture,
			sync::get_src_texture_access(src),
			sync::get_dst_texture_access(dst_access_type),
			base_mip, mips,
			base_layer, layers
		));
	};

	for (int l = 0; l < texture_range.layers; l++) {
		u32 layer = base_layer + l;

		// Run-Length Encoding
		u32 batch_count = 0;
		u32 batch_base = base_mip;
		TextureAccessType batch_src_access = TEXTURE_ACCESS_UNDEFINED;
		bool active_batch = false;

		for (int m = 0; m < texture_range.mips; m++) {
			u32 mip = base_mip + m;
			
			u32 idx = subresource_idx(mip, layer);

			TextureAccessType current_src = (TextureAccessType)t.subresource_states[idx];

			TextureAccess src_access = sync::get_src_texture_access(current_src);
			TextureAccess dst_access = sync::get_dst_texture_access(dst_access_type);
			
			bool needs_barrier =
				(src_access.layout != dst_access.layout) ||
				(sync::texture_access_is_write(current_src)) ||
				(sync::texture_access_is_write(dst_access_type));

			if (current_src == dst_access_type && !sync::texture_access_is_write(dst_access_type))
				needs_barrier = false;

			if (active_batch) {
				if (needs_barrier && current_src == batch_src_access) {
					batch_count++;
				} else {
					push_barrier(
						batch_src_access,
						batch_base, batch_count,
						layer, 1
					);

					if (needs_barrier) {
						batch_src_access = current_src;
						batch_base = mip;
						batch_count = 1;
						active_batch = true;
					} else {
						active_batch = false;
					}
				}
			} else if (needs_barrier) {
				batch_src_access = current_src;
				batch_base = mip;
				batch_count = 1;
				active_batch = true;
			}

			t.subresource_states[idx] = dst_access_type;
		}

		if (active_batch) {
			push_barrier(
				batch_src_access,
				batch_base, batch_count,
				layer, 1
			);
		}
	}
#endif

void RenderGraph::execute(
	CommandBuffer &cmd,
	const Swapchain &swapchain,
	RenderScene &scene, const Camera &camera,
	float delta_time, float elapsed_time
)
{
	if (stages.size() <= 0)
		return;

	for (int i = 0; i < stages.size(); i++) {
		RenderStage &stage = stages[i];

		if (stage.is_culled)
			continue;
		
		GFX_PROFILE_SCOPE(cmd, stage.name.c_str());

//		debug_log("Executing Render Stage: %s", stage.name.c_str());

		cmd.pipeline_barrier(
			0,
			stage.memory_barriers,
			stage.buffer_barriers,
			stage.texture_barriers
		);

		RenderStageResources stage_resources(*this, stage);

		RenderContext ctx = {
			.device = *device,
			.cache = *cache,
			.cmd = cmd,
			.scene = scene,
			.camera = camera,
			.delta_time = delta_time,
			.elapsed_time = elapsed_time
		};

		if (stage.type == RenderStage::TYPE_GRAPHICS) {
			cmd.begin_rendering(stage_resources.build_rendering_info());
			stage.record(ctx, stage_resources);
			cmd.end_rendering();
		} else {
			stage.record(ctx, stage_resources);
		}

		/*
		for (auto &in : stage.inputs) {
			RenderResource &r = resources[in.handle];

			if (!r.is_imported && r.last_stage_index == i) {
				switch (r.kind) {
					case RenderResource::KIND_TEXTURE: {
						pool.release_texture(r.physical_texture, r.texture_info);
						r.physical_texture = nullptr;
					} break;

					case RenderResource::KIND_BUFFER: {
						pool.release_buffer(r.physical_buffer, r.buffer_info);
						r.physical_buffer = nullptr;
					} break;
				}
			}
		}

		for (auto &out : stage.outputs) {
			RenderResource &r = resources[out.handle];

			if (!r.is_imported && r.last_stage_index + 1 == i) {
				switch (r.kind) {
					case RenderResource::KIND_TEXTURE: {
						pool.release_texture(r.physical_texture, r.texture_info);
						r.physical_texture = nullptr;
					} break;

					case RenderResource::KIND_BUFFER: {
						pool.release_buffer(r.physical_buffer, r.buffer_info);
						r.physical_buffer = nullptr;
					} break;
				}
			}
		}
		*/
	}
	
	present_to_swapchain(cmd, swapchain);

//	debug_log("");
}

void RenderGraph::present_to_swapchain(CommandBuffer &cmd, const Swapchain &swapchain)
{
	const RenderResource &backbuffer_resource = resources[backbuffer_handle];

	const Texture *swapchain_src_texture = backbuffer_resource.physical_texture;
	const Texture *swapchain_dst_texture = swapchain.get_current_texture();

	Vector<VkImageMemoryBarrier2> image_barriers = {
		sync::texture_memory_barrier(
			swapchain_src_texture,
			{ backbuffer_resource.tracking.pipeline_barrier_stage_flags, backbuffer_resource.tracking.to_flush_access },
			{ VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT },
			backbuffer_resource.tracking.layout,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		),
		sync::texture_memory_barrier(
			swapchain_dst_texture,
			{ VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE },
			{ VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT },
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		)
	};

	cmd.pipeline_barrier(0, {}, {}, image_barriers);

	VkImageBlit2 blit = {};
	blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	
	blit.srcOffsets[0] = { 0, 0, 0 };
	blit.srcOffsets[1] = { (int)swapchain_src_texture->get_width(), (int)swapchain_src_texture->get_height(), 1 };
	
	blit.dstOffsets[0] = { 0, 0, 0 };
	blit.dstOffsets[1] = { (int)swapchain_dst_texture->get_width(), (int)swapchain_dst_texture->get_height(), 1 };
	
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;

	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	cmd.blit(swapchain_src_texture, swapchain_dst_texture, { blit }, VK_FILTER_LINEAR);
	
	VkImageMemoryBarrier2 present_barrier = sync::texture_memory_barrier(
		swapchain_dst_texture,
		{ VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT },
		{ VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE },
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		0, 1,
		0, 1
	);

	cmd.pipeline_barrier(0, {}, {}, { present_barrier });
}

RenderResourceHandle RenderGraph::create_texture(const AttachmentInfo &info)
{
	RenderResourceHandle handle = resources.size();

	RenderResource resource = {};
	resource.kind = RenderResource::KIND_TEXTURE;
	resource.is_imported = false;

	resource.first_stage_index = -1u;
	resource.last_stage_index = -1u;

	resource.texture_info = info;
	
	resources.push_back(resource);

	return handle;
}

RenderResourceHandle RenderGraph::create_buffer(const GpuBufferInfo &info)
{
	RenderResourceHandle handle = resources.size();

	RenderResource resource = {};
	resource.kind = RenderResource::KIND_BUFFER;
	resource.is_imported = false;

	resource.first_stage_index = -1u;
	resource.last_stage_index = -1u;

	resource.buffer_info = info;
	resource.buffer_offset = 0;

	resources.push_back(resource);

	return handle;
}

RenderResourceHandle RenderGraph::import_texture(const Texture *texture)
{
	auto it = import_cache.find(texture);
	if (it != import_cache.end())
		return it->second;

	RenderResource resource = {};
	resource.kind = RenderResource::KIND_TEXTURE;
	resource.is_imported = true;

	resource.first_stage_index = -1u;
	resource.last_stage_index = -1u;

	resource.texture_info.size_class = SIZE_CLASS_ABSOLUTE;

	resource.texture_info.size_x = texture->get_width();
	resource.texture_info.size_y = texture->get_height();
	resource.texture_info.size_z = texture->get_depth();

	resource.texture_info.format  = texture->get_format();
	resource.texture_info.samples = texture->get_sample_count();
	resource.texture_info.mips    = texture->get_mipmap_count();
	resource.texture_info.layers  = texture->get_layer_count();

	resource.physical_texture = texture;

	auto state_it = tracked_external_textures.find(texture);
	if (state_it != tracked_external_textures.end()) {
		resource.tracking = tracked_external_textures[texture];
	} else {
		resource.tracking.pipeline_barrier_stage_flags = VK_PIPELINE_STAGE_2_NONE;
		resource.tracking.to_flush_access = VK_ACCESS_2_NONE;
		resource.tracking.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_zero_array(resource.tracking.invalidated_in_stage);
	}

	RenderResourceHandle handle = resources.size();

	resources.push_back(resource);

	import_cache[texture] = handle;

	return handle;
}

RenderResourceHandle RenderGraph::import_buffer(const GpuBuffer *buffer)
{
	auto it = import_cache.find(buffer);
	if (it != import_cache.end())
		return it->second;

	RenderResource resource = {};
	resource.kind = RenderResource::KIND_BUFFER;
	resource.is_imported = true;

	resource.first_stage_index = -1u;
	resource.last_stage_index = -1u;

	resource.buffer_info.flags = buffer->get_allocation_flags();
	resource.buffer_info.usage = buffer->get_usage();
	resource.buffer_info.size  = buffer->capacity();

	resource.physical_buffer = buffer;

	auto state_it = tracked_external_buffers.find(buffer);
	if (state_it != tracked_external_buffers.end()) {
		resource.tracking = tracked_external_buffers[buffer];
	} else {
		resource.tracking.pipeline_barrier_stage_flags = VK_PIPELINE_STAGE_2_NONE;
		resource.tracking.to_flush_access = VK_ACCESS_2_NONE;
		memory_zero_array(resource.tracking.invalidated_in_stage);
	}

	RenderResourceHandle handle = resources.size();

	resources.push_back(resource);

	import_cache[buffer] = handle;

	return handle;
}

#include "render_graph.h"

using namespace gfx;

RenderGraphBlackboard::RenderGraphBlackboard()
	: items{}
{
}

RenderGraphBlackboard::~RenderGraphBlackboard()
{
	for (auto &[id, item] : items)
		delete item;
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

	render_info.view_mask = stage.multi_view_mask;

	for (auto &output : stage.outputs) {
		RenderResource &resource = graph.resources[output.handle];

		const AttachmentInfo &attachment_info = resource.texture_info;

		// TODO: Right now it's just based on the last attachments sample count.
		//       Assumption is that all attachments will already have the same sample count.
		//       --> Ideally I should have resolving implemented so they automatically have their resolves.
		render_info.samples = attachment_info.samples;

		render_info.width = resource.texture_info.size_x;
		render_info.height = resource.texture_info.size_y;

		VkRenderingAttachmentInfo vk_attachment_info = {};
		vk_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

		vk_attachment_info.loadOp = output.texture.clear_enabled ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		vk_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		// TODO: SUCKS AND VERY SIMPLISTIC
		VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;

		if (resource.physical_texture->is_cubemap())
			view_type = VK_IMAGE_VIEW_TYPE_CUBE;

		vk_attachment_info.imageView = graph.get_device().fetch_texture_view(
			resource.physical_texture,
			view_type,
			output.texture.range.layers,
			output.texture.range.base_layer,
			output.texture.range.mips,
			output.texture.range.base_mip
		).get_handle();
		
		vk_attachment_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		
		// TODO: MSAA isn't supported yet.
		vk_attachment_info.resolveImageView = VK_NULL_HANDLE;
		vk_attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vk_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;

		const RenderClear &clear = output.texture.clear;

		if (attachment_info.format == graph.get_device().get_depth_format()) {
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

TextureView RenderStageResources::get_texture_view(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.resources[handle];
	SubresourceRange range;

	for (auto &in : stage.inputs) {
		if (in.handle == handle) {
			range = in.texture.range;
			break;
		}
	}

	// TODO: SUCKS AND VERY SIMPLISTIC
	VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;

	if (resource.physical_texture->is_cubemap())
		view_type = VK_IMAGE_VIEW_TYPE_CUBE;

	u32 layer_count = range.layers == VK_REMAINING_ARRAY_LAYERS ? resource.physical_texture->get_layer_count()  - range.base_mip   : range.mips;
	u32 mip_count   = range.mips   == VK_REMAINING_MIP_LEVELS   ? resource.physical_texture->get_mipmap_count() - range.base_layer : range.layers;

	return graph.get_device().fetch_texture_view(
		resource.physical_texture,
		view_type,
		layer_count,
		range.base_layer,
		mip_count,
		range.base_mip
	);
}

const GpuBuffer *RenderStageResources::get_buffer(RenderResourceHandle handle) const
{
	return graph.resources[handle].physical_buffer;
}

RenderGraphBuilder::RenderGraphBuilder(RenderGraph &graph, RenderStage &stage)
	: graph(graph)
	, current_stage(stage)
{
}

RenderGraphBuilder::~RenderGraphBuilder()
{
}

VkFormat RenderGraphBuilder::get_depth_format() const
{
	return graph.get_device().get_depth_format();
}

void RenderGraphBuilder::set_multi_view_mask(u32 mask)
{
	current_stage.multi_view_mask = mask;
}

RenderResourceHandle RenderGraphBuilder::create_texture(const AttachmentInfo &info) const
{
	RenderResourceHandle handle = graph.resources.size();

	RenderResource &resource = graph.resources.emplace_back();
	resource.kind = RenderResource::KIND_TEXTURE;
	resource.texture_info = info;
	resource.first_stage_index = current_stage.index;

	return handle;
}

RenderResourceHandle RenderGraphBuilder::create_buffer(const GpuBufferInfo &info) const
{
	RenderResourceHandle handle = graph.resources.size();

	RenderResource &resource = graph.resources.emplace_back();
	resource.kind = RenderResource::KIND_BUFFER;
	resource.buffer_info = info;
	resource.first_stage_index = current_stage.index;

	return handle;
}

RenderResourceHandle RenderGraphBuilder::import_texture(Texture *texture)
{
	return graph.import_texture(texture);
}

RenderResourceHandle RenderGraphBuilder::import_buffer(GpuBuffer *buffer)
{
	return graph.import_buffer(buffer);
}

RenderResourceHandle RenderGraphBuilder::write_colour(RenderResourceHandle handle, const SubresourceRange &range, const RenderClear *clear) const
{
	RenderResourceEdge &edge = current_stage.outputs.emplace_back();
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_COLOUR_ATTACHMENT;
	edge.texture.range = range;

	if (clear) {
		edge.texture.clear_enabled = true;
		edge.texture.clear = *clear;
	} else {
		edge.texture.clear_enabled = false;
	}

	return handle;
}

RenderResourceHandle RenderGraphBuilder::write_depth(RenderResourceHandle handle, const SubresourceRange &range, const RenderClear *clear) const
{
	RenderResourceEdge &edge = current_stage.outputs.emplace_back();
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_DEPTH_ATTACHMENT;
	edge.texture.range = range;

	if (clear) {
		edge.texture.clear_enabled = true;
		edge.texture.clear = *clear;
	} else {
		edge.texture.clear_enabled = false;
	}

	return handle;
}

RenderResourceHandle RenderGraphBuilder::read_texture(RenderResourceHandle handle, const SubresourceRange &range) const
{
	RenderResourceEdge &edge = current_stage.inputs.emplace_back();
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_SAMPLED;
	edge.texture.range = range;

	return handle;
}

RenderResourceHandle RenderGraphBuilder::blit_texture_src(RenderResourceHandle handle, const SubresourceRange &range) const
{
	RenderResourceEdge &edge = current_stage.inputs.emplace_back();
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_BLIT_SRC;
	edge.texture.range = range;

	return handle;
}

RenderResourceHandle RenderGraphBuilder::blit_texture_dst(RenderResourceHandle handle, const SubresourceRange &range) const
{
	RenderResourceEdge &edge = current_stage.outputs.emplace_back();
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_BLIT_DST;
	edge.texture.range = range;

	return handle;
}

RenderResourceHandle RenderGraphBuilder::write_buffer(RenderResourceHandle handle, GpuBufferAccessType usage)
{
	RenderResourceEdge &edge = current_stage.outputs.emplace_back();
	edge.handle = handle;
	edge.buffer.access = usage;

	return handle;
}

RenderResourceHandle RenderGraphBuilder::read_buffer(RenderResourceHandle handle, GpuBufferAccessType usage)
{
	RenderResourceEdge &edge = current_stage.inputs.emplace_back();
	edge.handle = handle;
	edge.buffer.access = usage;

	return handle;
}

RenderResourcePool::RenderResourcePool(RenderGraph &graph)
	: graph(graph)
	, current_frame(0)
{
}

RenderResourcePool::~RenderResourcePool()
{
}

void RenderResourcePool::flush(u64 frame_index)
{
	current_frame = frame_index;

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

const Texture *RenderResourcePool::acquire_texture(const AttachmentInfo &info)
{
	for (auto &t : texture_pool) {
		if (!t.in_use && t.texture_info == info) {
			t.in_use = true;
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

	texture.texture_info = info;
	texture.in_use = true;

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

const GpuBuffer *RenderResourcePool::acquire_buffer(const GpuBufferInfo &info)
{
	for (auto &b : buffer_pool) {
		if (!b.in_use && b.buffer_info == info) {
			b.in_use = true;
			return b.buffer;
		}
	}
	
	PooledBuffer buffer = {};

	buffer.buffer = graph.get_device().alloc_buffer(
		info.usage,
		info.flags,
		info.size
	);

	buffer.buffer_info = info;
	buffer.in_use = true;

	buffer_pool.push_back(buffer);

	return buffer.buffer;
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

RenderGraph::RenderGraph()
	: device(nullptr)
	, resources()
	, stages()
	, pool(*this)
	, import_cache()
	, backbuffer_handle(RENDER_INVALID_HANDLE)
{
}

RenderGraph::~RenderGraph()
{
}

void RenderGraph::init(Device *device)
{
	this->device = device;
}

void RenderGraph::destroy()
{
	pool.destroy();
	resources.clear();
	stages.clear();
	backbuffer_handle = RENDER_INVALID_HANDLE;
	import_cache.clear();
}

void RenderGraph::reset(u64 current_frame_index)
{
	stages.clear();
	resources.clear();
	backbuffer_handle = RENDER_INVALID_HANDLE;
	import_cache.clear();
	pool.flush(current_frame_index);
}

void RenderGraph::set_backbuffer_source(RenderResourceHandle handle)
{
	backbuffer_handle = handle;
}

void RenderGraph::compile(const Swapchain &swapchain)
{
	for (auto &r : resources) {
		r.first_stage_index = -1u;

		if (r.is_imported)
			r.ref_count = 1;
		else
			r.ref_count = 0;
	}

	if (backbuffer_handle != RENDER_INVALID_HANDLE)
		resources[backbuffer_handle].ref_count++;

	backpropogate_dependencies();

	allocate_resources(swapchain);

	generate_barriers();
}

void RenderGraph::backpropogate_dependencies()
{
	for (int i = stages.size() - 1; i >= 0; i--) {
		RenderStage &stage = stages[i];

		stage.is_culled = false;

		for (auto &out : stage.outputs) {
			/*
			if (out.handle == backbuffer_handle)
				stage.is_culled = false;

			if (resources[out.handle].ref_count > 0)
				stage.is_culled = false;
			*/

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

			switch (r.kind) {
				case RenderResource::KIND_TEXTURE: {
					if (r.physical_texture)
						continue;
					
					// Resolve rleative size.
					if (r.texture_info.size_class == SIZE_CLASS_SWAPCHAIN_RELATIVE) {
						r.texture_info.size_x *= swapchain.get_width();
						r.texture_info.size_y *= swapchain.get_height();
						r.texture_info.size_class = SIZE_CLASS_ABSOLUTE;
					}

					r.physical_texture = pool.acquire_texture(r.texture_info);
				} break;

				case RenderResource::KIND_BUFFER: {
					if (r.physical_buffer)
						continue;

					r.physical_buffer = pool.acquire_buffer(r.buffer_info);
				} break;
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
			process_edge(stage, in);

		for (auto &out : stage.outputs)
			process_edge(stage, out);
	}
}

void RenderGraph::process_edge(RenderStage &stage, const RenderResourceEdge &edge
)
{
	RenderResource &r = resources[edge.handle];
	
	switch (r.kind) {
		case RenderResource::KIND_TEXTURE: {
			if (!r.physical_texture)
				return;

			TextureAccess src_access = sync::get_src_texture_access(r.texture_access_type);
			TextureAccess dst_access = sync::get_dst_texture_access(edge.texture.access);

			bool needs_barrier =
				(src_access.layout != dst_access.layout) ||
				(sync::texture_access_is_write(edge.texture.access));

			needs_barrier = true; // TODO: temp

			if (needs_barrier) {
				stage.image_barriers.push_back(sync::texture_memory_barrier(
					r.physical_texture,
					src_access, dst_access,
					0, r.physical_texture->get_mipmap_count(),
					0, r.physical_texture->get_layer_count()
				));

				r.texture_access_type = edge.texture.access;
			}
		} break;

		case RenderResource::KIND_BUFFER: {
			if (!r.physical_buffer)
				return;

			GpuBufferAccess src_access = sync::get_src_buffer_access(r.buffer_access_type);
			GpuBufferAccess dst_access = sync::get_dst_buffer_access(edge.buffer.access);

			stage.buffer_barriers.push_back(sync::buffer_memory_barrier(
				r.physical_buffer,
				src_access, dst_access
			));

			r.buffer_access_type = edge.buffer.access;
		} break;
	}
}

void RenderGraph::execute(
	CommandBuffer &cmd,
	const Swapchain &swapchain,
	const SceneView &scene_view,
	float delta_time, float elapsed_time
)
{
	if (stages.size() <= 0)
		return;

	for (int i = 0; i < stages.size(); i++) {
		RenderStage &stage = stages[i];

		if (stage.is_culled)
			continue;

		cmd.pipeline_barrier(
			0, {},
			stage.buffer_barriers,
			stage.image_barriers
		);

		RenderStageResources stage_resources(*this, stage);

		RenderContext ctx = {
			.device = *device,
			.cmd = cmd,
			.scene_view = scene_view,
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

	/*
	for (auto &r : resources) {
		if (!r.is_imported) {
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

void RenderGraph::present_to_swapchain(CommandBuffer &cmd, const Swapchain &swapchain)
{
	const Texture *swapchain_src_texture = resources[backbuffer_handle].physical_texture;
	const Texture *swapchain_dst_texture = swapchain.get_current_texture();
	
	Vector<VkImageMemoryBarrier2> image_barriers = {
		sync::texture_memory_barrier(
			swapchain_src_texture,
			sync::get_src_texture_access(TEXTURE_ACCESS_COLOUR_ATTACHMENT),
			sync::get_dst_texture_access(TEXTURE_ACCESS_BLIT_SRC),
			0, swapchain_src_texture->get_mipmap_count(),
			0, swapchain_src_texture->get_layer_count()
		),
		sync::texture_memory_barrier(
			swapchain_dst_texture,
			sync::get_src_texture_access(TEXTURE_ACCESS_UNDEFINED),
			sync::get_dst_texture_access(TEXTURE_ACCESS_BLIT_DST),
			0, swapchain_dst_texture->get_mipmap_count(),
			0, swapchain_dst_texture->get_layer_count()
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
		sync::get_src_texture_access(TEXTURE_ACCESS_BLIT_DST),
		sync::get_dst_texture_access(TEXTURE_ACCESS_PRESENT),
		0, swapchain_dst_texture->get_mipmap_count(),
		0, swapchain_dst_texture->get_layer_count()
	);

	cmd.pipeline_barrier(0, {}, {}, { present_barrier });

	image_barriers.clear();
}

RenderResourceHandle RenderGraph::import_texture(const Texture *texture)
{
	if (import_cache.find(texture) != import_cache.end())
		return import_cache[texture];

	RenderResourceHandle handle = resources.size();
	
	RenderResource &res = resources.emplace_back();
	res.kind = RenderResource::KIND_TEXTURE;
	res.is_imported = true;

	res.physical_texture = texture;

	res.texture_info.format = texture->get_format();
	res.texture_info.samples = texture->get_sample_count();
	res.texture_info.mips = texture->get_mipmap_count();
	res.texture_info.layers = texture->get_layer_count();

	res.texture_info.size_class = SIZE_CLASS_ABSOLUTE;
	res.texture_info.size_x = texture->get_width();
	res.texture_info.size_y = texture->get_height();
	res.texture_info.size_z = texture->get_depth();

	import_cache[texture] = handle;

	return handle;
}

RenderResourceHandle RenderGraph::import_buffer(const GpuBuffer *buffer)
{
	if (import_cache.find(buffer) != import_cache.end())
		return import_cache[buffer];

	RenderResourceHandle handle = resources.size();

	RenderResource &resource = resources.emplace_back();
	resource.kind = RenderResource::KIND_BUFFER;
	resource.is_imported = true;

	resource.physical_buffer = buffer;
	
	resource.buffer_info.flags = buffer->get_allocation_flags();
	resource.buffer_info.usage = buffer->get_usage();
	resource.buffer_info.size = buffer->get_size();

	import_cache[buffer] = handle;

	return handle;
}

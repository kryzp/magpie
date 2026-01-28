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

		const Texture *physical_texture = (const Texture *)resource.physical_resource;
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

		if (physical_texture->is_cubemap())
			view_type = VK_IMAGE_VIEW_TYPE_CUBE;

		vk_attachment_info.imageView = graph.get_device().fetch_texture_view(physical_texture, view_type, output.texture.range).get_handle();
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
	return (const Texture *)graph.resources[handle].physical_resource;
}

TextureView RenderStageResources::get_texture_view(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.resources[handle];

	const Texture *physical_texture = (const Texture *)resource.physical_resource;
	
	SubresourceRange range = {};
	bool found = false;

	for (auto &in : stage.inputs) {
		if (in.handle == handle) {
			range = in.texture.range;
			found = true;
			break;
		}
	}

	assert(found);

	// TODO: SUCKS AND VERY SIMPLISTIC
	VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;

	if (physical_texture->is_cubemap())
		view_type = VK_IMAGE_VIEW_TYPE_CUBE;

	return graph.get_device().fetch_texture_view(physical_texture, view_type, range);
}

const GpuBuffer *RenderStageResources::get_buffer(RenderResourceHandle handle) const
{
	return (const GpuBuffer *)graph.resources[handle].physical_resource;
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

	RenderResource resource = {};
	resource.kind = RenderResource::KIND_TEXTURE;
	resource.is_imported = false;
	resource.initial_access = TEXTURE_ACCESS_UNDEFINED;
	resource.first_stage_index = current_stage.index;
	resource.last_stage_index = -1u;
	resource.texture_info = info;

	graph.resources.push_back(resource);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::create_buffer(const GpuBufferInfo &info) const
{
	RenderResourceHandle handle = graph.resources.size();

	RenderResource resource = {};
	resource.kind = RenderResource::KIND_BUFFER;
	resource.is_imported = false;
	resource.initial_access = GPU_BUFFER_ACCESS_UNDEFINED;
	resource.first_stage_index = current_stage.index;
	resource.last_stage_index = -1u;
	resource.buffer_info = info;

	graph.resources.push_back(resource);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::write_colour(RenderResourceHandle handle, const SubresourceRange &range, const RenderClear *clear) const
{
	RenderResourceEdge edge = {};
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_COLOUR_ATTACHMENT;
	edge.texture.range = range;

	if (clear) {
		edge.texture.clear_enabled = true;
		edge.texture.clear = *clear;
	} else {
		edge.texture.clear_enabled = false;
	}

	current_stage.outputs.push_back(edge);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::write_depth(RenderResourceHandle handle, const SubresourceRange &range, const RenderClear *clear) const
{
	RenderResourceEdge edge = {};
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_DEPTH_ATTACHMENT;
	edge.texture.range = range;

	if (clear) {
		edge.texture.clear_enabled = true;
		edge.texture.clear = *clear;
	} else {
		edge.texture.clear_enabled = false;
	}

	current_stage.outputs.push_back(edge);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::read_texture(RenderResourceHandle handle, const SubresourceRange &range) const
{
	RenderResourceEdge edge = {};
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_SAMPLED;
	edge.texture.range = range;

	current_stage.inputs.push_back(edge);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::blit_texture_src(RenderResourceHandle handle, const SubresourceRange &range) const
{
	RenderResourceEdge edge = {};
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_BLIT_SRC;
	edge.texture.range = range;

	current_stage.inputs.push_back(edge);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::blit_texture_dst(RenderResourceHandle handle, const SubresourceRange &range) const
{
	RenderResourceEdge edge = {};
	edge.handle = handle;
	edge.texture.access = TEXTURE_ACCESS_BLIT_DST;
	edge.texture.range = range;

	current_stage.outputs.push_back(edge);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::write_buffer(RenderResourceHandle handle, GpuBufferAccessType usage)
{
	RenderResourceEdge edge = {};
	edge.handle = handle;
	edge.buffer.access = usage;

	current_stage.outputs.push_back(edge);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::read_buffer(RenderResourceHandle handle, GpuBufferAccessType usage)
{
	RenderResourceEdge edge = {};
	edge.handle = handle;
	edge.buffer.access = usage;

	current_stage.inputs.push_back(edge);

	return handle;
}

RenderResourcePool::RenderResourcePool(RenderGraph &graph)
	: graph(graph)
{
}

RenderResourcePool::~RenderResourcePool()
{
}

void RenderResourcePool::flush()
{
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
		if (!t.in_use && t.info == info) {
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

	texture.info = info;
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
		if (!b.in_use && b.info == info) {
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

	buffer.info = info;
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
	, resource_access_cache()
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

void RenderGraph::reset()
{
	stages.clear();
	resources.clear();
	backbuffer_handle = RENDER_INVALID_HANDLE;
	import_cache.clear();
	pool.flush();
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
				if (r.physical_resource)
					continue;

				case RenderResource::KIND_TEXTURE:
					// Resolve rleative size.
					if (r.texture_info.size_class == SIZE_CLASS_SWAPCHAIN_RELATIVE) {
						r.texture_info.size_x *= swapchain.get_width();
						r.texture_info.size_y *= swapchain.get_height();
						r.texture_info.size_class = SIZE_CLASS_ABSOLUTE;
					}

					r.physical_resource = pool.acquire_texture(r.texture_info);
					break;

				case RenderResource::KIND_BUFFER:
					r.physical_resource = pool.acquire_buffer(r.buffer_info);
					break;
			}
		}
	}
}

void RenderGraph::generate_barriers()
{
	for (auto &r : resources)
		r.final_access = r.initial_access;

	for (auto &stage : stages) {
		if (stage.is_culled)
			continue;

		for (auto &in : stage.inputs)
			process_edge(stage, in);

		for (auto &out : stage.outputs)
			process_edge(stage, out);
	}
}

void RenderGraph::process_edge(RenderStage &stage, const RenderResourceEdge &edge)
{
	RenderResource &r = resources[edge.handle];
	
	if (!r.physical_resource)
		return;

	switch (r.kind) {
		case RenderResource::KIND_TEXTURE:
			transition_texture(stage, r, edge.texture.access, edge.texture.range);
			break;

		case RenderResource::KIND_BUFFER:
			transition_buffer(stage, r, edge.buffer.access);
			break;
	}
}

void RenderGraph::transition_texture(RenderStage &stage, RenderResource &t, TextureAccessType dst_access_type, const SubresourceRange &range)
{
#if 0
	/*
	 * Could this be optimized with a flood-fill style algorithm?
	 * Right now it just moves horizontally.
	 */

	int base_mip = range.base_mip;
	int base_layer = range.base_layer;

	for (int layer = 0; layer < range.layers; layer++) {
		TextureAccessType curr_src_access = t.texture.final_accesses[base_mip][base_layer + layer][0];
		t.texture.final_accesses[base_mip][base_layer + layer][0] = dst_access_type;

		int chain_length = 1;
		int curr_mip = 1;

		for (int mip = 1; mip < range.mips; mip++) {
			TextureAccessType new_src_access = t.texture.final_accesses[base_mip + mip][base_layer + layer][0];

			
		}
	}
#endif

	const Texture *physical_texture = (const Texture *)t.physical_resource;

	TextureAccess src_access = sync::get_src_texture_access((TextureAccessType)t.final_access);
	TextureAccess dst_access = sync::get_dst_texture_access(dst_access_type);

	bool needs_barrier =
		(src_access.layout != dst_access.layout) ||
		(sync::texture_access_is_write((TextureAccessType)t.final_access)) ||
		(sync::texture_access_is_write(dst_access_type));

	needs_barrier = true; // TODO: temp

	if (needs_barrier) {
		stage.texture_barriers.push_back(sync::texture_memory_barrier(
			physical_texture,
			src_access, dst_access,
			0, physical_texture->get_mipmap_count(),
			0, physical_texture->get_layer_count()
		));
		
		t.final_access = dst_access_type;
	}
}

void RenderGraph::transition_buffer(RenderStage &stage, RenderResource &b, GpuBufferAccessType dst_access_type)
{
	const GpuBuffer *physical_buffer = (const GpuBuffer *)b.physical_resource;

	GpuBufferAccess src_access = sync::get_src_buffer_access((GpuBufferAccessType)b.final_access);
	GpuBufferAccess dst_access = sync::get_dst_buffer_access(dst_access_type);

	stage.buffer_barriers.push_back(sync::buffer_memory_barrier(
		physical_buffer,
		src_access, dst_access
	));

	b.final_access = dst_access_type;
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
			stage.texture_barriers
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

	for (auto &r : resources) {
		if (!r.physical_resource || !r.is_imported)
			continue;

		resource_access_cache[r.physical_resource] = r.final_access;
	}
}

void RenderGraph::present_to_swapchain(CommandBuffer &cmd, const Swapchain &swapchain)
{
	const Texture *swapchain_src_texture = (const Texture *)resources[backbuffer_handle].physical_resource;
	const Texture *swapchain_dst_texture = swapchain.get_current_texture();
	
	Vector<VkImageMemoryBarrier2> image_barriers = {
		sync::texture_memory_barrier(
			swapchain_src_texture,
			sync::get_src_texture_access((TextureAccessType)resources[backbuffer_handle].final_access),
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
	
	RenderResource resource = {};
	resource.kind = RenderResource::KIND_TEXTURE;
	resource.is_imported = true;

	resource.first_stage_index = -1u;
	resource.last_stage_index = -1u;

	resource.physical_resource = texture;

	resource.initial_access = resource_access_cache.find(texture) != resource_access_cache.end()
		? resource_access_cache[texture]
		: TEXTURE_ACCESS_UNDEFINED;

	resource.texture_info.size_class = SIZE_CLASS_ABSOLUTE;
	resource.texture_info.size_x = texture->get_width();
	resource.texture_info.size_y = texture->get_height();
	resource.texture_info.size_z = texture->get_depth();

	resource.texture_info.format = texture->get_format();
	resource.texture_info.samples = texture->get_sample_count();
	resource.texture_info.mips = texture->get_mipmap_count();
	resource.texture_info.layers = texture->get_layer_count();

	resources.push_back(resource);

	import_cache[texture] = handle;

	return handle;
}

RenderResourceHandle RenderGraph::import_buffer(const GpuBuffer *buffer)
{
	if (import_cache.find(buffer) != import_cache.end())
		return import_cache[buffer];

	RenderResourceHandle handle = resources.size();

	RenderResource resource = {};
	resource.kind = RenderResource::KIND_BUFFER;
	resource.is_imported = true;

	resource.first_stage_index = -1u;
	resource.last_stage_index = -1u;

	resource.physical_resource = buffer;

	resource.initial_access = resource_access_cache.find(buffer) != resource_access_cache.end()
		? resource_access_cache[buffer]
		: GPU_BUFFER_ACCESS_UNDEFINED;

	resource.buffer_info.flags = buffer->get_allocation_flags();
	resource.buffer_info.usage = buffer->get_usage();
	resource.buffer_info.size = buffer->get_size();

	resources.push_back(resource);

	import_cache[buffer] = handle;

	return handle;
}

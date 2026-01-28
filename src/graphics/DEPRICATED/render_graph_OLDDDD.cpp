#include "render_graph.h"

#include "math/calc.h"

using namespace gfx;

static ResourceAttributes get_resource_attributes(const RenderResource &resource, const Swapchain &swapchain)
{
	ResourceAttributes att = {};
	att.kind = resource.kind;

	switch (resource.kind) {
		case RenderResource::KIND_TEXTURE: {
			const AttachmentInfo &info = resource.texture_info;
			info.get_absolute_size(swapchain, &att.texture.width, &att.texture.height, &att.texture.depth);
			att.texture.format = info.format;
			att.texture.samples = info.samples;
			att.texture.mips = info.mips;
			att.texture.layers = info.layers;
			att.texture.is_cubemap = info.is_cubemap;
			att.texture.is_transient = info.is_transient;
			att.texture.is_storage = info.is_storage;
		} break;

		case RenderResource::KIND_BUFFER: {
			const GpuBufferInfo &info = resource.buffer_info;
			att.buffer.size = info.size;
			att.buffer.flags = info.flags;
			att.buffer.usage = info.usage;
		} break;
	}

	return att;
}

RenderStageResources::RenderStageResources(RenderGraph &graph)
	: graph(graph)
	, active_handles()
	, external_cache()
	, external_textures()
	, external_texture_views()
	, external_buffers()
	, physical_attributes()
	, physical_textures()
	, physical_texture_views()
	, physical_buffers()
	, texture_pool()
	, buffer_pool()
{
}

RenderStageResources::~RenderStageResources()
{
}

void RenderStageResources::destroy()
{
	for (auto &t : texture_pool)
		graph.get_device().destroy_texture(t.texture);

	for (auto &b : buffer_pool)
		graph.get_device().destroy_buffer(b.buffer);

	texture_pool.clear();
	buffer_pool.clear();
}

void RenderStageResources::flush()
{
	for (auto &t : texture_pool)
		t.in_use = false;

	for (auto &b : buffer_pool)
		b.in_use = false;
}

u32 RenderStageResources::register_external_texture(const Texture &texture)
{
	if (external_cache.find(texture.get_handle()) != external_cache.end())
		return external_cache[texture.get_handle()];

	u32 physical_index = external_textures.size();

	external_textures.push_back(texture);
	external_texture_views.push_back(graph.get_device().fetch_texture_view_std(texture));
	
	external_cache[texture.get_handle()] = physical_index;
	return physical_index;
}

u32 RenderStageResources::register_external_buffer(const GpuBuffer &buffer)
{
	if (external_cache.find(buffer.get_handle()) != external_cache.end())
		return external_cache[buffer.get_handle()];

	u32 physical_index = physical_attributes.size();

	external_buffers.push_back(buffer);

	external_cache[buffer.get_handle()] = physical_index;
	return physical_index;
}

void RenderStageResources::compile(const Vector<RenderStage> &stages, RenderResourceHandle swapchain_src, const Swapchain &swapchain)
{
	active_handles.clear();
	physical_attributes.clear();
	
	for (auto &stage : stages) {
		for (auto &output : stage.outputs)
			resolve(output.handle, swapchain);

		for (auto &texture : stage.textures)
			resolve(texture, swapchain);

		for (auto &buffer : stage.buffers)
			resolve(buffer, swapchain);
	}

	if (swapchain_src.is_valid())
		resolve(swapchain_src, swapchain);

	alloc_resources();

	setup_aliases();
}

void RenderStageResources::resolve(RenderResourceHandle handle, const Swapchain &swapchain)
{
	RenderResource &resource = graph.get_resource(handle);

	if (resource.is_allocated())
		return;

	resource.physical_index = physical_attributes.size();
	physical_attributes.push_back(get_resource_attributes(resource, swapchain));

	active_handles.push_back(handle);
}

void RenderStageResources::alloc_resources()
{
	u32 count = physical_attributes.size();
	
	physical_textures.resize(count);
	physical_texture_views.resize(count);
	physical_buffers.resize(count);

	for (int i = 0; i < count; i++) {
		ResourceAttributes att = physical_attributes[i];
		
		switch (att.kind) {
			case RenderResource::KIND_TEXTURE: {
				physical_textures[i] = acquire_texture(att);
				physical_texture_views[i] = graph.get_device().fetch_texture_view_std(texture_pool[physical_textures[i]].texture);
			} break;

			case RenderResource::KIND_BUFFER: {
				physical_buffers[i] = acquire_buffer(att);
			} break;
		}
	}
}

u32 RenderStageResources::acquire_texture(const ResourceAttributes &att)
{
	for (int i = 0; i < texture_pool.size(); i++) {
		auto &t = texture_pool[i];
		if (!t.in_use && t.desc.compatible_with(att)) {
			t.in_use = true;
			return i;
		}
	}

	PooledTexture entry = {};

	entry.texture = graph.get_device().alloc_texture(
		att.texture.width,
		att.texture.height,
		att.texture.depth,
		att.texture.format,
		att.texture.depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		att.texture.mips,
		att.texture.layers,
		att.texture.samples,
		att.texture.is_transient,
		att.texture.is_storage,
		att.texture.is_cubemap
	);

	entry.desc = att;
	entry.in_use = true;

	texture_pool.push_back(entry);
	return texture_pool.size() - 1;
}

u32 RenderStageResources::acquire_buffer(const ResourceAttributes &att)
{
	for (int i = 0; i < buffer_pool.size(); i++) {
		auto &b = buffer_pool[i];
		if (!b.in_use && b.desc.compatible_with(att)) {
			b.in_use = true;
			return i;
		}
	}

	PooledBuffer entry = {};

	entry.buffer = graph.get_device().alloc_buffer(
		att.buffer.usage,
		att.buffer.flags,
		att.buffer.size
	);

	entry.desc = att;
	entry.in_use = true;

	buffer_pool.push_back(entry);
	return buffer_pool.size() - 1;
}

void RenderStageResources::setup_aliases()
{
	for (RenderResourceHandle handle : active_handles) {
		RenderResource &child = graph.get_resource(handle);

		if (!child.is_alias())
			continue;

		RenderResource &parent = graph.get_resource(child.alias_of.parent);

		// SHOULDN'T HAPPEN. PARENT SHOULD BE ALLOCATED FIRST!
		assert(parent.is_allocated());

		const Texture &src_texture = texture_pool[physical_textures[parent.physical_index]].texture;

		u32 mip_width = src_texture.get_width();
		u32 mip_height = src_texture.get_height();

		for (u32 m = 0; m < child.alias_of.range.base_mip; m++) {
			mip_width = CalcU::max(1u, mip_width >> 1);
			mip_height = CalcU::max(1u, mip_height >> 1);
		}

		physical_texture_views[child.physical_index] = graph.get_device().fetch_texture_view(
			src_texture,
			child.alias_of.view_type,
			child.alias_of.range.layers,
			child.alias_of.range.base_layer,
			child.alias_of.range.mips,
			child.alias_of.range.base_mip
		);
	}
}

Texture &RenderStageResources::get_texture(RenderResourceHandle handle)
{
	RenderResource &resource = graph.get_resource(handle);

	if (resource.is_imported)
		return external_textures[resource.physical_index];

	return texture_pool[physical_textures[resource.physical_index]].texture;
}

const Texture &RenderStageResources::get_texture(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.get_resource(handle);

	if (resource.is_imported)
		return external_textures[resource.physical_index];
	
	return texture_pool[physical_textures[resource.physical_index]].texture;
}

TextureView &RenderStageResources::get_texture_view(RenderResourceHandle handle)
{
	RenderResource &resource = graph.get_resource(handle);
	
	if (resource.is_imported)
		return external_texture_views[resource.physical_index];

	return physical_texture_views[resource.physical_index];
}

const TextureView &RenderStageResources::get_texture_view(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.get_resource(handle);
	
	if (resource.is_imported)
		return external_texture_views[resource.physical_index];

	return physical_texture_views[resource.physical_index];
}

GpuBuffer &RenderStageResources::get_buffer(RenderResourceHandle handle)
{
	RenderResource &resource = graph.get_resource(handle);

	if (resource.is_imported)
		return external_buffers[resource.physical_index];
	
	return buffer_pool[physical_buffers[resource.physical_index]].buffer;
}

const GpuBuffer &RenderStageResources::get_buffer(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.get_resource(handle);

	if (resource.is_imported)
		return external_buffers[resource.physical_index];
	
	return buffer_pool[physical_buffers[resource.physical_index]].buffer;
}

RenderGraphBuilder::RenderGraphBuilder(RenderGraph &graph, RenderStage &stage)
	: graph(graph)
	, current_stage(stage)
{
}

RenderGraphBuilder::~RenderGraphBuilder()
{
}

void RenderGraphBuilder::set_multi_view_mask(u32 mask)
{
	current_stage.multi_view_mask = mask;
}

void RenderGraphBuilder::set_record(const std::function<void(const RenderContext &ctx, const RenderStageResources &resources)> &record)
{
	current_stage.record = record;
}

/*
RenderResourceHandle RenderGraphBuilder::move_subresource(RenderResourceHandle parent, VkImageViewType type, const SubresourceRange &range) const
{
	SubresourceAlias alias = {};
	alias.parent = parent;
	alias.view_type = type;
	alias.range = range;

	return graph.move_subresource(alias);
}
*/

RenderResourceHandle RenderGraphBuilder::create_texture(const AttachmentInfo &info) const
{
	u32 index = graph.resources.size();

	RenderResource &res = graph.resources.emplace_back(RenderResource::KIND_TEXTURE, index);
	res.texture_info = info;
	res.is_imported = false;
	
	return RenderResourceHandle(index);
}

RenderResourceHandle RenderGraphBuilder::create_buffer(const GpuBufferInfo &info) const
{
	u32 index = graph.resources.size();

	RenderResource &res = graph.resources.emplace_back(RenderResource::KIND_BUFFER, index);
	res.buffer_info = info;
	res.is_imported = false;
	
	return RenderResourceHandle(index);
}

RenderResourceHandle RenderGraphBuilder::write_colour(RenderResourceHandle handle, const RenderClear *clear) const
{
	RenderResource &resource = graph.get_resource(handle);
	resource.texture_accesses.push_back(sync::TEXTURE_ACCESS_colour);

	RenderStage::OutputAttachment output_attachment = {};
	output_attachment.handle = handle;

	if (clear) {
		output_attachment.clear_enabled = true;
		output_attachment.clear = *clear;
	} else {
		output_attachment.clear_enabled = false;
	}

	current_stage.outputs.push_back(output_attachment);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::write_depth(RenderResourceHandle handle, const RenderClear *clear) const
{
	RenderResource &resource = graph.get_resource(handle);
	resource.texture_accesses.push_back(sync::TEXTURE_ACCESS_depth);

	RenderStage::OutputAttachment output_attachment = {};
	output_attachment.handle = handle;

	if (clear) {
		output_attachment.clear_enabled = true;
		output_attachment.clear = *clear;
	} else {
		output_attachment.clear_enabled = false;
	}

	current_stage.outputs.push_back(output_attachment);

	return handle;
}

RenderResourceHandle RenderGraphBuilder::read_texture(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.get_resource(handle);
	resource.texture_accesses.push_back(sync::TEXTURE_ACCESS_graphics_r);
	current_stage.textures.push_back(handle);
	return handle;
}

RenderResourceHandle RenderGraphBuilder::blit_texture_src(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.get_resource(handle);
	resource.texture_accesses.push_back(sync::TEXTURE_ACCESS_blit_src);
	current_stage.textures.push_back(handle);
	return handle;
}

RenderResourceHandle RenderGraphBuilder::blit_texture_dst(RenderResourceHandle handle) const
{
	RenderResource &resource = graph.get_resource(handle);
	resource.texture_accesses.push_back(sync::TEXTURE_ACCESS_blit_dst);
	current_stage.textures.push_back(handle);
	return handle;
}

RenderResourceHandle RenderGraphBuilder::add_buffer(RenderResourceHandle handle, sync::GpuBufferAccessType access) const
{
	RenderResource &resource = graph.get_resource(handle);
	resource.buffer_accesses.push_back(access);
	current_stage.buffers.push_back(handle);
	return handle;
}

RenderGraph &RenderGraphBuilder::get_graph()
{
	return graph;
}

const RenderGraph &RenderGraphBuilder::get_graph() const
{
	return graph;
}

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

RenderGraph::RenderGraph()
	: device(nullptr)
	, stages()
	, resources()
	, physical_resources(*this)
	, swapchain_source()
	, frame_import_cache()
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
	physical_resources.destroy();
}

void RenderGraph::execute(
	CommandBuffer &cmd,
	Swapchain &swapchain,
	SceneView scene_view,
	float delta_time, float elapsed_time
)
{
	assert(swapchain_source.is_valid());
	
	physical_resources.compile(stages, swapchain_source, swapchain);
	
	for (auto &stage : stages)
		execute_stage(stage, cmd, swapchain, scene_view, delta_time, elapsed_time);

	Texture &swapchain_src_texture = physical_resources.get_texture(swapchain_source);
	Texture &swapchain_dst_texture = swapchain.get_current_texture();
	
	Vector<VkImageMemoryBarrier2> image_barriers;

	transition_texture(swapchain_src_texture, sync::TEXTURE_ACCESS_blit_src, image_barriers);
	transition_texture(swapchain_dst_texture, sync::TEXTURE_ACCESS_blit_dst, image_barriers);

	cmd.pipeline_barrier(0, {}, {}, image_barriers);
	image_barriers.clear();

	VkImageBlit2 blit = {};
	blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

	blit.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
	blit.srcOffsets[1] = (VkOffset3D){ (int)swapchain_src_texture.get_width(), (int)swapchain_src_texture.get_height(), 1 };

	blit.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
	blit.dstOffsets[1] = (VkOffset3D){ (int)swapchain_dst_texture.get_width(), (int)swapchain_dst_texture.get_height(), 1 };

	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;

	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	cmd.blit(swapchain_src_texture, swapchain_dst_texture, { blit }, VK_FILTER_LINEAR);

	transition_texture(swapchain_dst_texture, sync::TEXTURE_ACCESS_present, image_barriers);

	cmd.pipeline_barrier(0, {}, {}, image_barriers);
	image_barriers.clear();
	
	stages.clear();
	resources.clear();
	frame_import_cache.clear();
	physical_resources.flush();
}

void RenderGraph::execute_stage(
	const RenderStage &stage,
	CommandBuffer &cmd, Swapchain &swapchain,
	SceneView scene_view,
	float delta_time, float elapsed_time
)
{
	Vector<VkImageMemoryBarrier2> image_barriers;
	Vector<VkBufferMemoryBarrier2> buffer_barriers;

	for (auto &output : stage.outputs) {
		Texture &gpu_texture = physical_resources.get_texture(output.handle);
		RenderResource &resource = get_resource(output.handle);
		sync::TextureAccessType access = resource.texture_accesses.front();
		resource.texture_accesses.pop_front();
		transition_texture(gpu_texture, access, image_barriers);
	}

	for (auto &texture : stage.textures) {
		Texture &gpu_texture = physical_resources.get_texture(texture);
		RenderResource &resource = get_resource(texture);
		sync::TextureAccessType access = resource.texture_accesses.front();
		resource.texture_accesses.pop_front();
		transition_texture(gpu_texture, access, image_barriers);
	}

	for (auto &buffer : stage.buffers) {
		GpuBuffer &gpu_buffer = physical_resources.get_buffer(buffer);
		RenderResource &resource = get_resource(buffer);
		sync::GpuBufferAccessType access = resource.buffer_accesses.front();
		resource.buffer_accesses.pop_front();
		transition_buffer(gpu_buffer, access, buffer_barriers);
	}

	cmd.pipeline_barrier(0, {}, buffer_barriers, image_barriers);

	RenderContext ctx = {
		.device = *device,
		.cmd = cmd,
		.view = scene_view,
		.delta_time = delta_time,
		.elapsed_time = elapsed_time
	};

	if (stage.type == RenderStage::TYPE_GRAPHICS) {
		RenderInfo render_info = {};
		render_info.view_mask = stage.multi_view_mask;

		for (auto &output : stage.outputs) {
			RenderResource &resource = get_resource(output.handle);

			const AttachmentInfo &attachment_info = resource.texture_info;

			// TODO: Right now it's just based on the last attachments sample count.
			//       Assumption is that all attachments will already have the same sample count.
			//       --> Ideally I should have resolving implemented so they automatically have their resolves.
			render_info.samples = attachment_info.samples;

			attachment_info.get_absolute_size(
				swapchain,
				&render_info.width,
				&render_info.height,
				nullptr
			);

			VkRenderingAttachmentInfo vk_attachment_info = {};
			vk_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

			vk_attachment_info.loadOp = output.clear_enabled ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			vk_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			
			vk_attachment_info.imageView = physical_resources.get_texture_view(output.handle).get_handle();
			vk_attachment_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			
			// TODO: MSAA isn't supported yet.
			vk_attachment_info.resolveImageView = VK_NULL_HANDLE;
			vk_attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vk_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;

			const RenderClear &clear = output.clear;

			if (attachment_info.format == device->get_depth_format()) {
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

		cmd.begin_rendering(render_info);

		if (stage.record)
			stage.record(ctx, physical_resources);

		cmd.end_rendering();
	} else {
		if (stage.record)
			stage.record(ctx, physical_resources);
	}
}

/*
RenderResourceHandle RenderGraph::move_subresource(const SubresourceAlias &alias)
{
	assert(alias.parent.is_valid());

	u32 index = resources.size();

	RenderResource &res = resources.emplace_back(RenderResource::KIND_TEXTURE, index);
	res.texture_info = get_resource(alias.parent).texture_info;
	res.alias_of = alias;

	assert(get_resource(alias.parent).kind == RenderResource::KIND_TEXTURE);

	return RenderResourceHandle(index);
}
*/

void RenderGraph::resize(u32 width, u32 height)
{
	// TODO
}

void RenderGraph::set_swapchain_source(const RenderResourceHandle &source)
{
	swapchain_source = source;
}

RenderResource &RenderGraph::get_resource(const RenderResourceHandle &handle)
{
	return resources[handle.index];
}

RenderResourceHandle RenderGraph::import_texture(const Texture &texture)
{
	if (frame_import_cache.find(&texture) != frame_import_cache.end())
		return frame_import_cache[&texture];

	u32 index = resources.size();
	
	RenderResource &res = resources.emplace_back(RenderResource::KIND_TEXTURE, index);
	res.is_imported = true;

	res.texture_info.format = texture.get_format();
	res.texture_info.samples = texture.get_sample_count();
	res.texture_info.mips = texture.get_mipmap_count();
	res.texture_info.layers = texture.get_layer_count();

	res.texture_info.size_class = SIZE_CLASS_ABSOLUTE;
	res.texture_info.size_x = texture.get_width();
	res.texture_info.size_y = texture.get_height();
	res.texture_info.size_z = texture.get_depth();
	
	res.physical_index = physical_resources.register_external_texture(texture);

	RenderResourceHandle handle(index);

	frame_import_cache[&texture] = handle;
	return handle;
}

RenderResourceHandle RenderGraph::import_buffer(const GpuBuffer &buffer)
{
	if (frame_import_cache.find(&buffer) != frame_import_cache.end())
		return frame_import_cache[&buffer];

	u32 index = resources.size();
	
	RenderResource &res = resources.emplace_back(RenderResource::KIND_BUFFER, index);
	res.is_imported = true;

	res.buffer_info.size = buffer.get_size();
	res.buffer_info.flags = buffer.get_allocation_flags();
	res.buffer_info.usage = buffer.get_usage();
	
	res.physical_index = physical_resources.register_external_buffer(buffer);

	RenderResourceHandle handle(index);

	frame_import_cache[&buffer] = handle;
	return handle;
}

void RenderGraph::transition_texture(Texture &texture, sync::TextureAccessType dst_access, Vector<VkImageMemoryBarrier2> &barriers)
{
	sync::TextureAccess dst_access_info = sync::get_dst_texture_access(dst_access);

	for (u32 i = 0; i < texture.get_mipmap_count(); i++) {
		for (u32 j = 0; j < texture.get_layer_count(); j++) {
			sync::TextureAccessType src_access = texture.get_access(i, j, 0);

			if (src_access == dst_access)
				continue;

			sync::TextureAccess src_access_info = sync::get_src_texture_access(src_access);

			texture.set_access(i, j, 0, dst_access);
			barriers.push_back(sync::texture_memory_barrier(texture, src_access_info, dst_access_info, i, 1, j, 1));
		}
	}
}

#if 0

static void stage_transition_view(struct gfx_texture_view *view,
				  enum gfx_texture_access_type dst_access,
				  VkImageMemoryBarrier2 *barriers, u32 *barrier_count)
{
	/*
	 * Could this be optimized with a flood-fill style algorithm?
	 * Right now it just moves horizontally.
	 */

	struct gfx_texture_access dst_access_info = gfx_sync_get_dst_texture_access(dst_access);

	for (u32 j = 0; j < view->layer_count; j++) {
		enum gfx_texture_access_type curr_src_access = gfx_texture_get_access_type(view->parent,
											   view->base_mip,
											   view->base_layer + j, 0);

		gfx_texture_set_access_type(view->parent,
					    view->base_mip,
					    view->base_layer + j, 0,
					    dst_access);

		u32 chain_length = 1;
		u32 curr_mip = 0;

		for (u32 i = 1; i < view->mip_count; i++) {
			enum gfx_texture_access_type new_src_access = gfx_texture_get_access_type(view->parent,
												  view->base_mip + i,
												  view->base_layer + j, 0);

			gfx_texture_set_access_type(view->parent,
						    view->base_mip + i,
						    view->base_layer + j, 0,
						    dst_access);

			if (curr_src_access == new_src_access) {
				// Continue the chain.
				chain_length++;
			} else {
				// Generate a new pipeline barrier.
				curr_src_access = new_src_access;

				barriers[*barrier_count] = gfx_sync_texture_memory_barrier(view->parent,
											   gfx_sync_get_src_texture_access(curr_src_access),
											   dst_access_info,
											   view->base_mip + curr_mip, chain_length,
											   view->base_layer + j, 1);

				*barrier_count = *barrier_count + 1;

				chain_length = 1;
				curr_mip = i;
			}
		}

		barriers[*barrier_count] = gfx_sync_texture_memory_barrier(view->parent,
									   gfx_sync_get_src_texture_access(curr_src_access),
									   dst_access_info,
									   view->base_mip + curr_mip, chain_length,
									   view->base_layer + j, 1);

		*barrier_count = *barrier_count + 1;
	}
}

#endif // 0

void RenderGraph::transition_buffer(GpuBuffer &buffer, sync::GpuBufferAccessType dst_access, Vector<VkBufferMemoryBarrier2> &barriers)
{
	sync::GpuBufferAccess src_access_info = sync::get_src_buffer_access(buffer.get_access_type());
	sync::GpuBufferAccess dst_access_info = sync::get_dst_buffer_access(dst_access);

	if (buffer.get_access_type() == dst_access)
		return;

	buffer.set_access_type(dst_access);
	barriers.push_back(sync::buffer_memory_barrier(buffer, src_access_info, dst_access_info));
}

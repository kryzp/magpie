#include "render_graph.h"

#include "core/hash.h"
#include "core/scratch.h"

using namespace gfx;

RenderStage::RenderStage(RenderGraph &graph, Type type)
	: graph(graph)
	, type(type)
	, scene_view()
	, multi_view_mask()
	, record()
	, outputs()
	, textures()
	, buffers()
{
}

RenderStage::~RenderStage()
{
}

const RenderStage::Type RenderStage::get_type() const
{
	return type;
}

const SceneView &RenderStage::get_view() const
{
	return scene_view;
}

u32 RenderStage::get_multi_view_mask() const
{
	return multi_view_mask;
}

const RenderStageRecordFn &RenderStage::get_record_fn() const
{
	return record;
}

const Vector<RenderStage::OutputAttachment> &RenderStage::get_output_attachments() const
{
	return outputs;
}

const Vector<RenderResourceHandle> &RenderStage::get_textures() const
{
	return textures;
}

const Vector<RenderResourceHandle> &RenderStage::get_buffers() const
{
	return buffers;
}

void RenderStage::set_scene_view(const SceneView &view)
{
	this->scene_view = view;
}

void RenderStage::set_multi_view_mask(u32 mask)
{
	this->multi_view_mask = mask;
}

void RenderStage::set_record(const RenderStageRecordFn &fn)
{
	this->record = fn;
}

void RenderStage::add_colour_output(const RenderResourceHandle &handle, const RenderClear *clear)
{
	RenderResource &resource = graph.get_resource(handle);
	resource.texture_accesses.push_back(sync::TEXTURE_ACCESS_colour);

	OutputAttachment output_attachment = {};
	output_attachment.handle = handle;

	if (clear)
		output_attachment.clear = *clear;

	outputs.push_back(output_attachment);
}

void RenderStage::add_depth_stencil_output(const RenderResourceHandle &handle, const RenderClear *clear)
{
	RenderResource &resource = graph.get_resource(handle);
	resource.texture_accesses.push_back(sync::TEXTURE_ACCESS_depth);

	OutputAttachment output_attachment = {};
	output_attachment.handle = handle;

	if (clear)
		output_attachment.clear = *clear;

	outputs.push_back(output_attachment);
}

void RenderStage::add_buffer_output(const RenderResourceHandle &handle, sync::GpuBufferAccessType access)
{
	RenderResource &resource = graph.get_resource(handle);
	resource.buffer_accesses.push_back(access);
	buffers.push_back(handle);
}

void RenderStage::add_buffer_input(const RenderResourceHandle &handle, sync::GpuBufferAccessType access)
{
	RenderResource &resource = graph.get_resource(handle);
	resource.buffer_accesses.push_back(access);
	textures.push_back(handle);
}

void RenderStage::add_texture(const RenderResourceHandle &handle, sync::TextureAccessType access)
{
	RenderResource &resource = graph.get_resource(handle);
	resource.texture_accesses.push_back(access);
	textures.push_back(handle);
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
	, physical_slot_used()
	, physical_attributes()
	, physical_textures()
	, physical_buffers()
	, swapchain_source(nullptr)
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
	for (auto &resource : resources) {
		if (resource.physical_index != RENDER_INVALID_INDEX && !resource.is_alias()) {
			switch (resource.kind) {
				case RenderResource::KIND_TEXTURE:  device->destroy_texture(physical_textures[resource.physical_index]); break;
				case RenderResource::KIND_BUFFER:   device->destroy_gpu_buffer(physical_buffers[resource.physical_index]); break;
			}
		}
	}

	resources.clear();
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

void RenderGraph::execute(
	CommandBuffer &cmd, Swapchain &swapchain,
	float delta_time, float elapsed_time
)
{
	assert(swapchain_source);
	
	build_physical_resources(swapchain);
	setup_attachments();
	setup_aliases();
	
	// This handles the case where swapchain source
	// is only used for presentation and not actually
	// referenced in any of the stages.
	if (swapchain_source->physical_index == RENDER_INVALID_INDEX) {
        swapchain_source->physical_index = physical_attributes.size();
        physical_attributes.push_back(get_resource_attributes(*swapchain_source, swapchain));
		physical_slot_used.resize(physical_attributes.size());
		physical_textures.resize(physical_attributes.size());
		physical_texture_views.resize(physical_attributes.size());
        setup_physical_texture(swapchain_source->physical_index);
	}

	for (auto &stage : stages)
		execute_stage(stage, cmd, swapchain, delta_time, elapsed_time);

	stages.clear();
}

void RenderGraph::execute_stage(
	const RenderStage &stage,
	CommandBuffer &cmd, Swapchain &swapchain,
	float delta_time, float elapsed_time
)
{
	Vector<VkImageMemoryBarrier2> image_barriers;
	Vector<VkBufferMemoryBarrier2> buffer_barriers;

	for (auto &output : stage.get_output_attachments()) {
		RenderResource &resource = get_resource(output.handle);
		Texture &gpu_texture = get_physical_texture(resource);
		sync::TextureAccessType access = resource.texture_accesses.front();
		resource.texture_accesses.pop_front();
		transition_texture(gpu_texture, access, image_barriers);
	}

	for (auto &texture : stage.get_textures()) {
		RenderResource &resource = get_resource(texture);
		Texture &gpu_texture = get_physical_texture(resource);
		sync::TextureAccessType access = resource.texture_accesses.front();
		resource.texture_accesses.pop_front();
		transition_texture(gpu_texture, access, image_barriers);
	}

	for (auto &buffer : stage.get_buffers()) {
		RenderResource &resource = get_resource(buffer);
		GpuBuffer &gpu_buffer = get_physical_buffer(resource);
		sync::GpuBufferAccessType access = resource.buffer_accesses.front();
		resource.buffer_accesses.pop_front();
		transition_buffer(gpu_buffer, access, buffer_barriers);
	}

	if (swapchain_source && stage.get_type() == RenderStage::TYPE_PRESENT) {
		// TODO: This fucking sucks ass.
		Texture &swapchain_src_texture = physical_textures[swapchain_source->physical_index];
		Texture &swapchain_dst_texture = swapchain.get_current_texture();

		transition_texture(swapchain_src_texture, sync::TEXTURE_ACCESS_blit_src, image_barriers);
		transition_texture(swapchain_dst_texture, sync::TEXTURE_ACCESS_blit_dst, image_barriers);

		cmd.pipeline_barrier(0, {}, buffer_barriers, image_barriers);
		image_barriers.clear();
		buffer_barriers.clear();

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
	}

	cmd.pipeline_barrier(0, {}, buffer_barriers, image_barriers);

	RenderContext ctx = {
		.device = *device,
		.cmd = cmd,
		.view = stage.get_view(),
		.delta_time = delta_time,
		.elapsed_time = elapsed_time
	};

	if (stage.get_type() == RenderStage::TYPE_GRAPHICS) {
		RenderInfo render_info = {};
		render_info.view_mask = stage.get_multi_view_mask();

		for (auto &output : stage.get_output_attachments()) {
			RenderResource &resource = get_resource(output.handle);

			const AttachmentInfo &attachment_info = resource.texture_info;
			const RenderClear &clear = output.clear;

			// TODO: Right now it's just based on the last attachments sample count.
			//       Assumption is that all attachments will already have the same sample count.
			//       --> Ideally I should have resolving implemented so they automatically have their resolves.
			render_info.samples = attachment_info.samples;

			attachment_info.get_absolute_size(swapchain,
				&render_info.width, &render_info.height
			);

			VkRenderingAttachmentInfo vk_attachment_info = {};
			vk_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

			vk_attachment_info.loadOp = clear.enabled ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			vk_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			
			vk_attachment_info.imageView = physical_texture_views[resource.physical_index].get_handle();
			vk_attachment_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			
			// TODO: MSAA isn't supported yet.
			vk_attachment_info.resolveImageView = VK_NULL_HANDLE;
			vk_attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vk_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;

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

		if (stage.get_record_fn())
			stage.get_record_fn()(ctx);

		cmd.end_rendering();
	} else {
		if (stage.get_record_fn())
			stage.get_record_fn()(ctx);
	}
}

RenderStage &RenderGraph::add_stage(RenderStage::Type type)
{
	return stages.emplace_back(*this, type);
}

void RenderGraph::resize(u32 width, u32 height)
{
}

void RenderGraph::set_swapchain_source(const RenderResourceHandle &source)
{
	swapchain_source = &get_resource(source);
}

void RenderGraph::move_subresource(const RenderResourceHandle &child, const SubresourceAlias &alias)
{
	RenderResource &parent_resource = get_resource(alias.parent);
	RenderResource &child_resource = get_resource(child);

	assert(parent_resource.kind == RenderResource::KIND_TEXTURE);
	assert(child_resource.kind == RenderResource::KIND_TEXTURE);

	child_resource.alias_of = alias;

	/*
	child_resource.texture_info = parent_resource.texture_info;
    
    if (alias.base_mip > 0)
        child_resource.texture_info.mips = 1;
    
    if (alias.view_type == VK_IMAGE_VIEW_TYPE_CUBE || 
        alias.view_type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        child_resource.texture_info.is_cubemap = true;
        child_resource.texture_info.layers = 6;
    }
	*/
}

ResourceAttributes RenderGraph::get_resource_attributes(const RenderResource &resource, const Swapchain &swapchain)
{
	ResourceAttributes att = {};
	att.kind = resource.kind;
	att.resource_index = resource.index;

	switch (resource.kind) {
		case RenderResource::KIND_TEXTURE: {
			const AttachmentInfo &info = resource.texture_info;
			info.get_absolute_size(swapchain, &att.texture.width, &att.texture.height);
			att.texture.format = info.format;
			att.texture.depth = 1;
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

RenderResourceHandle RenderGraph::create_texture_resource(const AttachmentInfo &info)
{
	u32 index = resources.size();
	RenderResource &resource = resources.emplace_back(RenderResource::KIND_TEXTURE, index);
	resource.texture_info = info;

	RenderResourceHandle handle;
	handle.index = index;

	return handle;
}

RenderResourceHandle RenderGraph::create_buffer_resource(const GpuBufferInfo &info)
{
	u32 index = resources.size();
	RenderResource &resource = resources.emplace_back(RenderResource::KIND_BUFFER, index);
	resource.buffer_info = info;

	RenderResourceHandle handle;
	handle.index = index;

	return handle;
}

RenderResource &RenderGraph::get_resource(const RenderResourceHandle &handle)
{
	return resources[handle.index];
}

Texture &RenderGraph::get_physical_texture(const RenderResource &resource)
{
	return physical_textures[resource.physical_index];
}

const Texture &RenderGraph::get_physical_texture(const RenderResource &resource) const
{
	return physical_textures[resource.physical_index];
}

TextureView &RenderGraph::get_physical_texture_view(const RenderResource &resource)
{
	return physical_texture_views[resource.physical_index];
}

const TextureView &RenderGraph::get_physical_texture_view(const RenderResource &resource) const
{
	return physical_texture_views[resource.physical_index];
}

GpuBuffer &RenderGraph::get_physical_buffer(const RenderResource &resource)
{
	return physical_buffers[resource.physical_index];
}

const GpuBuffer &RenderGraph::get_physical_buffer(const RenderResource &resource) const
{
	return physical_buffers[resource.physical_index];
}

void RenderGraph::build_physical_resources(const Swapchain &swapchain)
{	
	auto setup_physical_index = [&](const RenderResourceHandle &handle) -> void {
		RenderResource &resource = get_resource(handle);
		
		bool is_alias = resource.is_alias();
		bool is_allocated = resource.physical_index != RENDER_INVALID_INDEX;

		if (!is_allocated && !is_alias) {
			resource.physical_index = physical_attributes.size();
			physical_attributes.push_back(get_resource_attributes(resource, swapchain));
		} else if (is_alias) {
			RenderResource &parent_resource = get_resource(resource.alias_of.parent);

			// If the parent doesn't exist yet then create it.
			if (parent_resource.physical_index == RENDER_INVALID_INDEX) {
				parent_resource.physical_index = physical_attributes.size();
				physical_attributes.push_back(get_resource_attributes(parent_resource, swapchain));
			}

			resource.physical_index = parent_resource.physical_index;
		} else {
			physical_attributes[resource.physical_index] = get_resource_attributes(resource, swapchain);
		}
	};

	for (auto &stage : stages) {
		for (auto &output : stage.get_output_attachments())
			setup_physical_index(output.handle);

		for (auto &texture : stage.get_textures())
			setup_physical_index(texture);

		for (auto &buffer : stage.get_buffers())
			setup_physical_index(buffer);
	}
}

void RenderGraph::setup_physical_texture(u32 index)
{
	if (physical_slot_used[index])
		return;

	const ResourceAttributes &att = physical_attributes[index];

	physical_textures[index] = device->alloc_texture(
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

	physical_texture_views[index] = device->fetch_texture_view_std(physical_textures[index]);

	physical_slot_used[index] = true;
}

void RenderGraph::setup_physical_buffer(u32 index)
{
	if (physical_slot_used[index])
		return;
	
	const ResourceAttributes &att = physical_attributes[index];

	physical_buffers[index] = device->alloc_gpu_buffer(
		att.buffer.usage,
		att.buffer.flags,
		att.buffer.size
	);

	physical_slot_used[index] = true;
}

void RenderGraph::setup_attachments()
{
	physical_slot_used.resize(physical_attributes.size());
	physical_textures.resize(physical_attributes.size());
	physical_texture_views.resize(physical_attributes.size());
	physical_buffers.resize(physical_attributes.size());

	for (int i = 0; i < physical_attributes.size(); i++) {
		const auto &att = physical_attributes[i];
		switch (att.kind) {
			case RenderResource::KIND_TEXTURE:  setup_physical_texture(i); break;
			case RenderResource::KIND_BUFFER:   setup_physical_buffer(i); break;
		}
	}
}

void RenderGraph::setup_aliases()
{
    for (int i = 0; i < resources.size(); i++) {
        RenderResource &child = resources[i];

		if (!child.is_alias())
			continue;

		RenderResource &parent = get_resource(child.alias_of.parent);
		
		// SHOULDN'T HAPPEN. PARENT SHOULD BE ALLOCATED FIRST!
		assert(parent.physical_index != RENDER_INVALID_INDEX);
		
		Texture &src_texture = physical_textures[parent.physical_index];
		
		u32 mip_width = src_texture.get_width();
		u32 mip_height = src_texture.get_height();
		
		for (u32 m = 0; m < child.alias_of.base_mip; m++) {
			mip_width = std::max(1u, mip_width >> 1);
			mip_height = std::max(1u, mip_height >> 1);
		}
		
		physical_texture_views[child.physical_index] = device->fetch_texture_view(
			src_texture,
			child.alias_of.view_type,
			child.texture_info.layers,
			child.alias_of.base_layer,
			child.alias_of.base_mip
		);
		
		physical_slot_used[child.physical_index] = true;
    }
}

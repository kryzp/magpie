#include "render/graph/graph.h"
#include "graphics/device.h"
#include "graphics/sync.h"
#include "core/maths.h"

rnd::GraphTexture *rnd::Graph::TextureFromHandle(GraphTexHandle handle)
{
	DebugLogAssert(log_channel, !handle.IsNull(), "Handle is null.");
	DebugLogAssert(log_channel, handle.value <= texture_versions.size(), "Handle version is invalid.");

	unsigned ver_index = handle.value - 1;
	unsigned res_index = texture_versions[ver_index].resource_index;

	DebugLogAssert(log_channel, res_index < texture_resources.size(), "Resource index is invalid.");

	return &texture_resources[res_index];
}

rnd::GraphBuffer *rnd::Graph::BufferFromHandle(GraphBufHandle handle)
{
	DebugLogAssert(log_channel, !handle.IsNull(), "Handle is null.");
	DebugLogAssert(log_channel, handle.value <= buffer_versions.size(), "Handle version is invalid.");

	unsigned ver_index = handle.value - 1;
	unsigned res_index = buffer_versions[ver_index].resource_index;

	DebugLogAssert(log_channel, res_index < buffer_resources.size(), "Resource index is invalid.");

	return &buffer_resources[res_index];
}

const rnd::GraphTexture *rnd::Graph::TextureFromHandle(GraphTexHandle handle) const
{
	DebugLogAssert(log_channel, !handle.IsNull(), "Handle is null.");
	DebugLogAssert(log_channel, handle.value <= texture_versions.size(), "Handle version is invalid.");

	unsigned ver_index = handle.value - 1;
	unsigned res_index = texture_versions[ver_index].resource_index;

	DebugLogAssert(log_channel, res_index < texture_resources.size(), "Resource index is invalid.");

	return &texture_resources[res_index];
}

const rnd::GraphBuffer *rnd::Graph::BufferFromHandle(GraphBufHandle handle) const
{
	DebugLogAssert(log_channel, !handle.IsNull(), "Handle is null.");
	DebugLogAssert(log_channel, handle.value <= buffer_versions.size(), "Handle version is invalid.");

	unsigned ver_index = handle.value - 1;
	unsigned res_index = buffer_versions[ver_index].resource_index;

	DebugLogAssert(log_channel, res_index < buffer_resources.size(), "Resource index is invalid.");

	return &buffer_resources[res_index];
}

bool rnd::Graph::TexVersionIsUnwritten(GraphTexHandle handle) const
{
	DebugLogAssert(log_channel, !handle.IsNull(), "Handle is null.");
	return texture_versions[handle.value - 1].writer_pass == RENDER_GRAPH_INVALID_INDEX;
}

bool rnd::Graph::BufVersionIsUnwritten(GraphBufHandle handle) const
{
	DebugLogAssert(log_channel, !handle.IsNull(), "Handle is null.");
	return buffer_versions[handle.value - 1].writer_pass == RENDER_GRAPH_INVALID_INDEX;
}

rnd::Graph::Graph(LogChannel log_channel_)
	: log_channel(log_channel_)
	, passes()
	, pool()
	, tracker()
	, backbuffer_handle()
	, present_filter(VK_FILTER_LINEAR)
	, texture_resources()
	, texture_versions()
	, buffer_resources()
	, buffer_versions()
	, imported_textures()
	, imported_buffers()
{
	DebugLogI(log_channel, "Initialized.");
}

rnd::Graph::~Graph()
{
	FreePassData();

	DebugLogI(log_channel, "Destroyed.");
}

void rnd::Graph::Reset()
{
	for (auto &t : texture_resources) {
		if (t.is_imported)
			tracker.SetTexture(t.imported_key, t.state);
		else if (!t.physical_key.IsNull())
			pool.UpdateTexture(t.physical_key, t.state);
	}

	for (auto &b : buffer_resources) {
		if (b.is_imported)
			tracker.SetBuffer(b.imported_key, b.state);
		else if (!b.physical_key.IsNull())
			pool.UpdateBuffer(b.physical_key, b.state);
	}

	backbuffer_handle = GraphTexHandle();

	passes.clear();

	texture_resources.clear();
	texture_versions.clear();

	buffer_resources.clear();
	buffer_versions.clear();

	imported_textures.clear();
	imported_buffers.clear();

	pool.Flush();

	FreePassData();
}

void rnd::Graph::FreePassData()
{
	for (const PassData &data : pass_data)
		data.destroy(data.ptr);

	pass_data.clear();
}

rnd::Pass *rnd::Graph::Add(const char *name, PassType type)
{
	Pass pass;
	pass.graph = this;
	pass.name = name;
	pass.type = type;
	pass.index = passes.size();

	passes.push_back(pass);

	return &passes[passes.size() - 1];
}

void rnd::Graph::SetBackbuffer(GraphTexHandle handle)
{
	backbuffer_handle = handle;
}

void rnd::Graph::SetPresentFilter(VkFilter filter)
{
	present_filter = filter;
}

rnd::GraphTexHandle rnd::Graph::CreateTexture(const TextureInfo &info)
{
	GraphTexture texture;
	texture.texture_info = info;
	texture.first_pass_index = RENDER_GRAPH_INVALID_INDEX;
	texture.last_pass_index = RENDER_GRAPH_INVALID_INDEX;
	texture.ref_count = 0;
	texture.is_imported = false;

	GraphTexVersion version;
	version.resource_index = texture_resources.size();
	version.writer_pass = RENDER_GRAPH_INVALID_INDEX;
	version.parent = RENDER_GRAPH_INVALID_INDEX;

	texture_resources.push_back(texture);
	texture_versions.push_back(version);

	GraphTexHandle handle;
	handle.value = texture_versions.size();

	return handle;
}

rnd::GraphBufHandle rnd::Graph::CreateBuffer(const BufferInfo &info)
{
	GraphBuffer buffer;
	buffer.buffer_info = info;
	buffer.first_pass_index = RENDER_GRAPH_INVALID_INDEX;
	buffer.last_pass_index = RENDER_GRAPH_INVALID_INDEX;
	buffer.ref_count = 0;
	buffer.is_imported = false;

	GraphBufVersion version;
	version.resource_index = buffer_resources.size();
	version.writer_pass = RENDER_GRAPH_INVALID_INDEX;
	version.parent = RENDER_GRAPH_INVALID_INDEX;

	buffer_resources.push_back(buffer);
	buffer_versions.push_back(version);

	GraphBufHandle handle;
	handle.value = buffer_versions.size();

	return handle;
}

rnd::GraphTexHandle rnd::Graph::ImportTexture(gfx::ResourceKey external_key)
{
	gfx::Device *device = gfx::Device::Get();

	for (auto &t : imported_textures) {
		if ((t.external_key == external_key))
			return t.handle;
	}

	const gfx::Texture *physical = device->TextureFromKey(external_key);

	DebugLogAssert(log_channel, physical, "Imported texture with key %llu is invalid.", external_key.value);

	GraphTexture texture;
	texture.texture_info.format = physical->format;
	texture.texture_info.size_class = SizeClass_Absolute;
	texture.texture_info.size_x = physical->width;
	texture.texture_info.size_y = physical->height;
	texture.texture_info.size_z = physical->depth;
	texture.texture_info.mips = physical->mipmap_count;
	texture.texture_info.layers = physical->layer_count;
	texture.texture_info.samples = physical->sample_count;

	/*
	texture->texture_info.is_cubemap = (physical->flags & G_TextureFlag_Cubemap) != 0;
	texture->texture_info.is_transient = (physical->flags & G_TextureFlag_Transient) != 0;
	texture->texture_info.is_storage = (physical->flags & G_TextureFlag_Storage) != 0;
	*/

	texture.physical_key = external_key;
	texture.is_imported = true;
	texture.imported_key = external_key;
	texture.first_pass_index = RENDER_GRAPH_INVALID_INDEX;
	texture.last_pass_index = RENDER_GRAPH_INVALID_INDEX;
	texture.ref_count = 0;

	const ResourceState *tracked = tracker.FindTexture(external_key);

	if (tracked) {
		texture.state = *tracked;
	} else {
		texture.state = ResourceState();
		texture.state.write_stage = VK_PIPELINE_STAGE_2_NONE;
		texture.state.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	GraphTexVersion version;
	version.resource_index = texture_resources.size();
	version.writer_pass = RENDER_GRAPH_INVALID_INDEX;
	version.parent = RENDER_GRAPH_INVALID_INDEX;

	texture_resources.push_back(texture);
	texture_versions.push_back(version);

	GraphTexHandle handle;
	handle.value = texture_versions.size();

	GraphImportedTexture entry;
	entry.external_key = external_key;
	entry.handle = handle;

	imported_textures.push_back(entry);

	return handle;
}

rnd::GraphBufHandle rnd::Graph::ImportBuffer(gfx::ResourceKey external_key)
{
	gfx::Device *device = gfx::Device::Get();

	for (auto &b : imported_buffers) {
		if ((b.external_key == external_key))
			return b.handle;
	}

	const gfx::Buffer *physical = device->BufferFromKey(external_key);

	DebugLogAssert(log_channel, physical, "Imported buffer with key %llu is invalid.", static_cast<u64>(external_key.value));

	GraphBuffer buffer;

	buffer.buffer_info.size = physical->size;
	buffer.buffer_info.flags = physical->allocation_flags;
	buffer.buffer_info.usage = physical->usage;

	buffer.physical_key = external_key;
	buffer.is_imported = true;
	buffer.imported_key = external_key;
	buffer.first_pass_index = RENDER_GRAPH_INVALID_INDEX;
	buffer.last_pass_index = RENDER_GRAPH_INVALID_INDEX;
	buffer.ref_count = 0;

	const ResourceState *tracked = tracker.FindBuffer(external_key);

	if (tracked) {
		buffer.state = *tracked;
	} else {
		buffer.state = ResourceState();
		buffer.state.write_stage = VK_PIPELINE_STAGE_2_NONE;
	}

	GraphBufVersion version;
	version.resource_index = buffer_resources.size();
	version.writer_pass = RENDER_GRAPH_INVALID_INDEX;
	version.parent = RENDER_GRAPH_INVALID_INDEX;

	buffer_resources.push_back(buffer);
	buffer_versions.push_back(version);

	GraphBufHandle handle;
	handle.value = buffer_versions.size();

	GraphImportedBuffer entry;
	entry.external_key = external_key;
	entry.handle = handle;

	imported_buffers.push_back(entry);

	return handle;
}

rnd::GraphTexHandle rnd::Graph::PushTexVersion(GraphTexHandle parent, unsigned writer_pass_index)
{
	DebugLogAssert(log_channel, !parent.IsNull(), "Parent handle is null.");
	unsigned parent_ver_index = parent.value - 1;
	DebugLogAssert(log_channel, parent_ver_index < texture_versions.size(), "Parent version is invalid.");

	GraphTexVersion version;
	version.resource_index = texture_versions[parent_ver_index].resource_index;
	version.writer_pass = writer_pass_index;
	version.parent = parent_ver_index;

	texture_versions.push_back(version);

	GraphTexHandle handle;
	handle.value = texture_versions.size();
	return handle;
}

rnd::GraphBufHandle rnd::Graph::PushBufVersion(GraphBufHandle parent, unsigned writer_pass_index)
{
	DebugLogAssert(log_channel, !parent.IsNull(), "Parent handle is null.");
	unsigned parent_ver_index = parent.value - 1;
	DebugLogAssert(log_channel, parent_ver_index < buffer_versions.size(), "Parent version is invalid.");

	GraphBufVersion version;
	version.resource_index = buffer_versions[parent_ver_index].resource_index;
	version.writer_pass = writer_pass_index;
	version.parent = parent_ver_index;

	buffer_versions.push_back(version);

	GraphBufHandle handle;
	handle.value = buffer_versions.size();
	return handle;
}

void rnd::Graph::Compile(const gfx::Swapchain *swapchain)
{
	for (auto &t : texture_resources) {
		if (t.is_imported)
			t.ref_count = 1;
	}

	for (auto &b : buffer_resources) {
		if (b.is_imported)
			b.ref_count = 1;
	}

	if (!backbuffer_handle.IsNull())
		TextureFromHandle(backbuffer_handle)->ref_count++;

	PropagateDependencies();
	BackpropagateDependencies();
	AllocateResources(swapchain);
	GenerateBarriers();
}

void rnd::Graph::PropagateDependencies()
{
	for (unsigned i = 0; i < passes.size(); i++) {
		Pass *pass = &passes[i];

		for (auto &out : pass->input_textures) {
			GraphTexture *t = TextureFromHandle(out.handle);
			if (t->first_pass_index == RENDER_GRAPH_INVALID_INDEX)
				t->first_pass_index = i;
			if (!out.resolve_handle.IsNull()) {
				GraphTexture *rt = TextureFromHandle(out.resolve_handle);
				if (rt->first_pass_index == RENDER_GRAPH_INVALID_INDEX)
					rt->first_pass_index = i;
			}
		}

		for (auto &out : pass->output_buffers) {
			GraphBuffer *b = BufferFromHandle(out.handle);
			if (b->first_pass_index == RENDER_GRAPH_INVALID_INDEX)
				b->first_pass_index = i;
		}

		if (pass->is_culled)
			continue;

		for (auto &inp : pass->input_textures) {
			GraphTexture *t = TextureFromHandle(inp.handle);
			if (t->first_pass_index == RENDER_GRAPH_INVALID_INDEX)
				t->first_pass_index = i;
		}

		for (auto &inp : pass->input_buffers) {
			GraphBuffer *b = BufferFromHandle(inp.handle);
			if (b->first_pass_index == RENDER_GRAPH_INVALID_INDEX)
				b->first_pass_index = i;
		}
	}
}

void rnd::Graph::BackpropagateDependencies()
{
	for (int i = static_cast<int>(passes.size()) - 1; i >= 0; i--) {
		Pass *pass = &passes[i];

		pass->is_culled = true;

		for (auto &out : pass->output_textures) {

			GraphTexture *t = TextureFromHandle(out.handle);

			if (out.handle == backbuffer_handle)
				pass->is_culled = false;

			if (t->ref_count > 0)
				pass->is_culled = false;

			if (t->last_pass_index == RENDER_GRAPH_INVALID_INDEX)
				t->last_pass_index = i;

			if (!out.resolve_handle.IsNull()) {
				GraphTexture *rt = TextureFromHandle(out.resolve_handle);

				if (out.resolve_handle == backbuffer_handle)
					pass->is_culled = false;

				if (rt->ref_count > 0)
					pass->is_culled = false;

				if (rt->last_pass_index == RENDER_GRAPH_INVALID_INDEX)
					rt->last_pass_index = i;
			}
		}

		for (auto &out : pass->output_buffers) {

			GraphBuffer *b = BufferFromHandle(out.handle);

			if (b->ref_count > 0)
				pass->is_culled = false;

			if (b->last_pass_index == RENDER_GRAPH_INVALID_INDEX)
				b->last_pass_index = i;
		}

		if (pass->is_culled)
			continue;

		/*
		 * Basically:
		 * There's a bug regarding dependencies when it comes
		 * to ping-pong resources. If I have resources A and B, and I have a pass
		 * write to A, then copy A -> B, and only B gets used from then on, the
		 * pass writing to A would get culled because it's transient dependency isn't propagated.
		 * This fixes it by just bumping the reference count of parent resources.
		 */
		for (auto &out : pass->output_textures) {
			unsigned ver_idx = out.handle.value - 1;
			unsigned parent_idx = texture_versions[ver_idx].parent;

			if (parent_idx != RENDER_GRAPH_INVALID_INDEX) {
				GraphTexture *t = TextureFromHandle(out.handle);
				t->ref_count++;
			}

			if (!out.resolve_handle.IsNull()) {
				unsigned r_ver_idx = out.resolve_handle.value - 1;
				unsigned r_parent_idx = texture_versions[r_ver_idx].parent;

				if (r_parent_idx != RENDER_GRAPH_INVALID_INDEX) {
					GraphTexture *rt = TextureFromHandle(out.resolve_handle);
					rt->ref_count++;
				}
			}
		}

		for (auto &inp : pass->input_textures) {
			GraphTexture *t = TextureFromHandle(inp.handle);
			t->ref_count++;
			if (t->last_pass_index == RENDER_GRAPH_INVALID_INDEX)
				t->last_pass_index = i;
		}

		for (auto &inp : pass->input_buffers) {
			GraphBuffer *b = BufferFromHandle(inp.handle);
			b->ref_count++;
			if (b->last_pass_index == RENDER_GRAPH_INVALID_INDEX)
				b->last_pass_index = i;
		}
	}
}

void rnd::Graph::AllocateResources(const gfx::Swapchain *swapchain)
{
	for (auto &pass : passes) {

		if (pass.is_culled)
			continue;

		for (auto &out : pass.output_textures) {
			GraphTexture *t = TextureFromHandle(out.handle);

			if (!t->is_imported && t->physical_key.IsNull()) {
				if (t->texture_info.size_class == SizeClass_SwapchainRelative) {
					t->texture_info.size_x *= static_cast<float>(swapchain->width);
					t->texture_info.size_y *= static_cast<float>(swapchain->height);
					t->texture_info.size_class = SizeClass_Absolute;
				} else if (t->texture_info.size_class == SizeClass_Relative) {
					GraphTexHandle rel = t->texture_info.relative_to;

					DebugLogAssert(
						log_channel,
						!rel.IsNull(),
						"Cannot have SizeClass_Relative with a null relative_to handle."
					);

					GraphTexture *base = TextureFromHandle(rel);

					DebugLogAssert(
						log_channel,
						base->texture_info.size_class == SizeClass_Absolute,
						"Base texture (relative to) must already be resolved to an absolute size."
					);

					t->texture_info.size_x *= base->texture_info.size_x;
					t->texture_info.size_y *= base->texture_info.size_y;
					t->texture_info.size_z *= base->texture_info.size_z;

					t->texture_info.size_class = SizeClass_Absolute;
				}

				t->physical_key = pool.AcquireTexture(t->texture_info, &t->state);
			}

			if (!out.resolve_handle.IsNull()) {
				GraphTexture *rt = TextureFromHandle(out.resolve_handle);

				if (!rt->is_imported && rt->physical_key.IsNull()) {
					if (rt->texture_info.size_class == SizeClass_SwapchainRelative) {
						rt->texture_info.size_x *= static_cast<float>(swapchain->width);
						rt->texture_info.size_y *= static_cast<float>(swapchain->height);
						rt->texture_info.size_class = SizeClass_Absolute;
					} else if (rt->texture_info.size_class == SizeClass_Relative) {
						GraphTexHandle rel = rt->texture_info.relative_to;

						DebugLogAssert(
							log_channel,
							!rel.IsNull(),
							"Cannot have SizeClass_Relative with a null relative_to handle."
						);

						GraphTexture *base = TextureFromHandle(rel);

						DebugLogAssert(
							log_channel,
							base->texture_info.size_class == SizeClass_Absolute,
							"Base texture (relative to) must already be resolved to an absolute size."
						);

						rt->texture_info.size_x *= base->texture_info.size_x;
						rt->texture_info.size_y *= base->texture_info.size_y;
						rt->texture_info.size_z *= base->texture_info.size_z;

						rt->texture_info.size_class = SizeClass_Absolute;
					}

					rt->physical_key = pool.AcquireTexture(rt->texture_info, &rt->state);
				}
			}
		}

		for (auto &out : pass.output_buffers) {
			GraphBuffer *b = BufferFromHandle(out.handle);

			if (b->is_imported)
				continue;

			if (!b->physical_key.IsNull())
				continue;

			b->physical_key = pool.AcquireBuffer(b->buffer_info, &b->state);
		}
	}
}

void rnd::Graph::GenerateBarriers()
{
	for (auto &pass : passes) {

		if (pass.is_culled)
			continue;


		// Reads

		for (auto &t : pass.input_textures)
			SyncTextureRead(pass, t);

		for (auto &b : pass.input_buffers)
			SyncBufferRead(pass, b);


		// Writes

		for (auto &t : pass.output_textures) {
			SyncTextureWrite(pass, t);

			/*
			 * G_CmdEndRendering causes an implicit write to the resolution
			 * texture as well so we need to give it a barrier also.
			 */

			if (!t.resolve_handle.IsNull()) {

				PassTextureEdge resolve_edge = t;

				resolve_edge.handle = t.resolve_handle;
				resolve_edge.layout = t.resolve_layout;

				resolve_edge.state.stage =
					VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

				resolve_edge.state.access =
					VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
					VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

				resolve_edge.resolve_handle = GraphTexHandle();
				resolve_edge.should_clear = false;

				SyncTextureWrite(pass, resolve_edge);
			}
		}

		for (auto &b : pass.output_buffers)
			SyncBufferWrite(pass, b);
	}
}

// pooled and imported textures start every frame as UNDEFINED.
// however the previous frames in flight may still be touching the same memory,
// therefore the first transition waits on everything that has alreaby been submitted.
static VkPipelineStageFlags2 PriorStage(const rnd::ResourceState *st, VkPipelineStageFlags2 known)
{
	if (known != 0)
		return known;

	if (st->layout == VK_IMAGE_LAYOUT_UNDEFINED)
		return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	return VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
}

void rnd::Graph::SyncTextureRead(Pass &pass, const PassTextureEdge &edge)
{
	gfx::Device *device = gfx::Device::Get();

	GraphTexture *t = TextureFromHandle(edge.handle);

	if (t->physical_key.IsNull())
		return;

	ResourceState *st = &t->state;

	const VkImageLayout target_layout = VK_IMAGE_LAYOUT_GENERAL;
	bool layout_change = st->layout != target_layout;

	if (st->write_access != 0 || layout_change) {

		// RAW HAZARD

		gfx::AccessState src = {};
		src.stage = PriorStage(st, st->write_stage);
		src.access = st->write_access;

		const gfx::Texture *physical = device->TextureFromKey(t->physical_key);

		/*
		 * YES I DO THE BARRIER FOR THE ENTIRE TEXTURE.
		 * IT MAKES STATE TRACKING INFINITELY EASIER, AND ULTIMATELY
		 * THE COST OF SIMPLY DECLARING YOULL BE READING AND WRITING
		 * TO A TEXTURE IS NOT THAT GREAT TO JUSTIFY A BUGGY PAIN IN
		 * THE ASS ACCESS STATE TRACKING SYSTEM FOR EVERY SUBCOMPONENT
		 * OF THE TEXTURE. THANK YOU AND GOODNIGHT.
		 */
		pass.texture_barriers.push_back(gfx::sync::TextureBarrier(
			physical,
			src, edge.state,
			st->layout, target_layout,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		));

		st->layout = target_layout;
		st->write_access = 0;
		st->read_stages = edge.state.stage;
	} else {
		st->read_stages |= edge.state.stage;
	}
}

void rnd::Graph::SyncTextureWrite(Pass &pass, const PassTextureEdge &edge)
{
	gfx::Device *device = gfx::Device::Get();

	GraphTexture *t = TextureFromHandle(edge.handle);

	if (t->physical_key.IsNull())
		return;

	ResourceState *st = &t->state;

	const VkImageLayout target_layout = VK_IMAGE_LAYOUT_GENERAL;
	bool layout_change = st->layout != target_layout;

	if (st->write_access != 0) {

		// WAW HAZARD

		gfx::AccessState src = {};
		src.stage = PriorStage(st, st->write_stage);
		src.access = st->write_access;

		const gfx::Texture *physical = device->TextureFromKey(t->physical_key);

		pass.texture_barriers.push_back(gfx::sync::TextureBarrier(
			physical,
			src, edge.state,
			st->layout, target_layout,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		));

		st->layout = target_layout;
	} else if (st->read_stages != 0 || layout_change) {

		// WAR HAZARD

		gfx::AccessState src_access = {};
		src_access.stage = PriorStage(st, st->read_stages);
		src_access.access = VK_ACCESS_2_NONE;

		if (st->read_stages == 0 && st->layout == VK_IMAGE_LAYOUT_UNDEFINED) {
			// first use this frame: last frame's writes must be made available too
			src_access.access = VK_ACCESS_2_MEMORY_WRITE_BIT;
		}

		gfx::AccessState dst_access = {};
		dst_access.stage = edge.state.stage;
		dst_access.access = layout_change ? edge.state.access : VK_ACCESS_2_NONE;

		const gfx::Texture *physical = device->TextureFromKey(t->physical_key);

		pass.texture_barriers.push_back(gfx::sync::TextureBarrier(
			physical,
			src_access,
			dst_access,
			st->layout, target_layout,
			0, VK_REMAINING_MIP_LEVELS,
			0, VK_REMAINING_ARRAY_LAYERS
		));

		st->layout = target_layout;
	}

	st->write_stage = edge.state.stage;
	st->write_access = edge.state.access & GRAPHICS_SYNC_WRITE_ACCESS_MASK;
	st->read_stages = 0;
}

void rnd::Graph::SyncBufferRead(Pass &pass, const PassBufferEdge &edge)
{
	gfx::Device *device = gfx::Device::Get();

	GraphBuffer *b = BufferFromHandle(edge.handle);

	if (b->physical_key.IsNull())
		return;

	ResourceState *st = &b->state;

	if (st->write_access != 0) {

		// RAW HAZARD

		gfx::AccessState src_access = {};
		src_access.stage = st->write_stage ? st->write_stage : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src_access.access = st->write_access;

		const gfx::Buffer *physical = device->BufferFromKey(b->physical_key);

		pass.buffer_barriers.push_back(gfx::sync::BufferBarrier(
			physical,
			src_access, edge.state,
			edge.offset,
			edge.size ? edge.size : physical->size
		));

		st->write_access = 0;
		st->read_stages = edge.state.stage;
	} else {
		st->read_stages |= edge.state.stage;
	}
}

void rnd::Graph::SyncBufferWrite(Pass &pass, const PassBufferEdge &edge)
{
	gfx::Device *device = gfx::Device::Get();

	GraphBuffer *b = BufferFromHandle(edge.handle);

	if (b->physical_key.IsNull())
		return;

	ResourceState *st = &b->state;

	if (st->write_access != 0) {

		// WAW HAZARD

		gfx::AccessState src_access = {};
		src_access.stage = st->write_stage ? st->write_stage : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src_access.access = st->write_access;

		const gfx::Buffer *physical = device->BufferFromKey(b->physical_key);

		pass.buffer_barriers.push_back(gfx::sync::BufferBarrier(
			physical,
			src_access,
			edge.state,
			edge.offset,
			edge.size ? edge.size : physical->size
		));

	} else if (st->read_stages != 0) {

		// WAR HAZARD

		gfx::AccessState src_access = {};
		src_access.stage = st->read_stages;
		src_access.access = VK_ACCESS_2_NONE;

		gfx::AccessState dst_access = {};
		dst_access.stage = edge.state.stage;
		dst_access.access = VK_ACCESS_2_NONE;

		const gfx::Buffer *physical = device->BufferFromKey(b->physical_key);

		pass.buffer_barriers.push_back(gfx::sync::BufferBarrier(
			physical,
			src_access,
			dst_access,
			edge.offset,
			edge.size ? edge.size : physical->size
		));
	}

	st->write_stage = edge.state.stage;
	st->write_access = edge.state.access & GRAPHICS_SYNC_WRITE_ACCESS_MASK;
	st->read_stages = 0;
}

void rnd::Graph::Execute(const gfx::CmdBuffer &cmd)
{
	for (auto &pass : passes) {

		/*
		  DebugLogD(log_channel,
		  "Executing Pass: %.*s, Culled: %s",
		  String8VArg(pass->name),
		  pass->is_culled ? "YES" : "NO");
		*/

		if (pass.is_culled)
			continue;

		cmd.PipelineBarrier(
			0,
			pass.memory_barriers.size(), pass.memory_barriers.data(),
			pass.buffer_barriers.size(), pass.buffer_barriers.data(),
			pass.texture_barriers.size(), pass.texture_barriers.data()
		);

		PassContext ctx = {};
		ctx.graph = this;
		ctx.cmd = &cmd;
		ctx.render_info = nullptr;
		ctx.user_data = pass.user_data;

		cmd.BeginLabel(pass.name);

		if (pass.type == PassType_Graphics) {
			gfx::RenderInfo render_info = BuildRenderingInfo(pass);

			cmd.BeginRendering(render_info);

			ctx.render_info = &render_info;

			if (pass.record)
				pass.record(ctx);

			cmd.EndRendering();
		} else {
			if (pass.record)
				pass.record(ctx);
		}

		cmd.EndLabel();
	}
}

void rnd::Graph::PresentToSwapchain(const gfx::Swapchain *swapchain, const gfx::CmdBuffer &cmd)
{
	gfx::Device *device = gfx::Device::Get();

	if (backbuffer_handle.IsNull())
		return;

	GraphTexture *backbuffer = TextureFromHandle(backbuffer_handle);

	gfx::ResourceKey src_texture_key = backbuffer->physical_key;
	gfx::ResourceKey dst_texture_key = swapchain->frames[swapchain->current_frame_index].texture_key;

	const gfx::Texture *src_texture = device->TextureFromKey(src_texture_key);
	const gfx::Texture *dst_texture = device->TextureFromKey(dst_texture_key);

	// Backbuffer = Transfer Source
	// Swapchain  = Trandfer Destination

	gfx::AccessState src_src = { backbuffer->state.write_stage, backbuffer->state.write_access };
	gfx::AccessState src_dst = { VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT };

	VkImageMemoryBarrier2 pre_blit_barriers[2] = {};

	pre_blit_barriers[0] = gfx::sync::TextureBarrier(
		src_texture,
		src_src,
		src_dst,
		backbuffer->state.layout,
		VK_IMAGE_LAYOUT_GENERAL,
		0, VK_REMAINING_MIP_LEVELS,
		0, VK_REMAINING_ARRAY_LAYERS
	);

	gfx::AccessState dst_src = { VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE };
	gfx::AccessState dst_dst = { VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };

	pre_blit_barriers[1] = gfx::sync::TextureBarrier(
		dst_texture,
		dst_src,
		dst_dst,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		0, VK_REMAINING_MIP_LEVELS,
		0, VK_REMAINING_ARRAY_LAYERS
	);

	cmd.PipelineBarrier(
		0,
		0, nullptr,
		0, nullptr,
		ArraySize(pre_blit_barriers), pre_blit_barriers
	);

	// Blit the backbuffer to the swapchain.

	VkImageBlit2 blit = {};
	blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

	blit.srcOffsets[0] = VkOffset3D{ 0, 0, 0 };
	blit.srcOffsets[1] = VkOffset3D{ static_cast<int>(src_texture->width), static_cast<int>(src_texture->height), 1 };

	blit.dstOffsets[0] = VkOffset3D{ 0, 0, 0 };
	blit.dstOffsets[1] = VkOffset3D{ static_cast<int>(dst_texture->width), static_cast<int>(dst_texture->height), 1 };

	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;

	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	cmd.Blit(src_texture_key, dst_texture_key, 1, &blit, present_filter);

	// Recorded so the next frame that reuses this texture waits for the blit.
	backbuffer->state.layout = VK_IMAGE_LAYOUT_GENERAL;
	backbuffer->state.write_stage = 0;
	backbuffer->state.write_access = 0;
	backbuffer->state.read_stages = VK_PIPELINE_STAGE_2_BLIT_BIT;

	// Transition swapchain to present.

	gfx::AccessState present_src = { VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
	gfx::AccessState present_dst = { VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE };

	VkImageMemoryBarrier2 present_barrier = gfx::sync::TextureBarrier(
		dst_texture,
		present_src,
		present_dst,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		0, 1,
		0, 1
	);

	cmd.PipelineBarrier(
		0,
		0, nullptr,
		0, nullptr,
		1, &present_barrier
	);
}

gfx::ResourceKey rnd::Graph::ResolveTexture(GraphTexHandle handle) const
{
	const GraphTexture *t = TextureFromHandle(handle);
	return t->physical_key;
}

gfx::ResourceKey rnd::Graph::ResolveTextureView(GraphTexHandle handle, gfx::SubresourceRange range) const
{
	gfx::Device *device = gfx::Device::Get();

	const GraphTexture *t = TextureFromHandle(handle);

	const gfx::Texture *physical = device->TextureFromKey(t->physical_key);

	gfx::TextureViewCreateInfo create_info = {};
	create_info.texture = t->physical_key;
	create_info.type = physical->DefaultViewType();
	create_info.range = range;

	return device->TextureViewFetch(create_info);
}

gfx::ResourceKey rnd::Graph::ResolveBuffer(GraphBufHandle handle) const
{
	const GraphBuffer *b = BufferFromHandle(handle);
	return b->physical_key;
}

rnd::BufferRange rnd::Graph::ResolveBufferRange(GraphBufHandle handle) const
{
	const GraphBuffer *b = BufferFromHandle(handle);

	BufferRange range = {};
	range.buffer = b->physical_key;
	range.size = b->buffer_info.size;
	range.offset = 0;

	return range;
}

gfx::RenderInfo rnd::Graph::BuildRenderingInfo(const Pass &pass) const
{
	gfx::Device *device = gfx::Device::Get();

	gfx::RenderInfo render_info = {};

	render_info.view_mask = pass.multi_view_mask;

	for (auto &out : pass.output_textures) {
		const GraphTexture *t = TextureFromHandle(out.handle);
		const TextureInfo *info = &t->texture_info;

		// TODO: right now its just based on the last attachments sample count.
		//       Assumption is that all attachments will already have the same sample count.
		//       --> ideally there should be implicit resolving per attachment when
		//           get around to finally implementing msaa n stuff.
		render_info.samples = info->samples;

		render_info.width = maths::Max(1u, static_cast<unsigned>(info->size_x) >> out.attachment_range.base_mip);
		render_info.height = maths::Max(1u, static_cast<unsigned>(info->size_y) >> out.attachment_range.base_mip);

		gfx::ResourceKey view = ResolveTextureView(out.handle, out.attachment_range);

		VkRenderingAttachmentInfo vk_info = {};
		vk_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

		vk_info.loadOp = out.should_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		vk_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		vk_info.imageView = device->TextureViewFromKey(view)->vk_handle;
		vk_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		if (!out.resolve_handle.IsNull()) {
			gfx::ResourceKey resolve_view = ResolveTextureView(out.resolve_handle, out.attachment_range);

			vk_info.resolveImageView = device->TextureViewFromKey(resolve_view)->vk_handle;
			vk_info.resolveImageLayout = out.resolve_layout;
			vk_info.resolveMode = out.resolve_mode;
		} else {
			vk_info.resolveImageView = VK_NULL_HANDLE;
			vk_info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			vk_info.resolveMode = VK_RESOLVE_MODE_NONE;
		}

		if (gfx::TextureFormatIsDepth(info->format)) {
			vk_info.clearValue.depthStencil.depth = out.clear.depth;
			vk_info.clearValue.depthStencil.stencil = out.clear.stencil;

			render_info.depth_attachment = vk_info;
			render_info.depth_attachment_format = info->format;
			render_info.has_depth_attachment = true;
		} else {
			vk_info.clearValue.color.float32[0] = out.clear.r;
			vk_info.clearValue.color.float32[1] = out.clear.g;
			vk_info.clearValue.color.float32[2] = out.clear.b;
			vk_info.clearValue.color.float32[3] = out.clear.a;

			DebugLogAssert(
				log_channel,
				render_info.colour_attachment_count < ArraySize(render_info.colour_attachments),
				"Exceeded max number of colour attachments (%u) in render info.",
				ArraySize(render_info.colour_attachments)
			);

			render_info.colour_attachment_formats[render_info.colour_attachment_count] = info->format;
			render_info.colour_attachments[render_info.colour_attachment_count] = vk_info;

			render_info.colour_attachment_count++;
		}
	}

	return render_info;
}

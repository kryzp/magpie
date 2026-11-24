#ifndef RENDER_GRAPH_PASS_H
#define RENDER_GRAPH_PASS_H

#include "core/types.h"
#include "graphics/sync.h"
#include "graphics/texture.h"
#include "graphics/pipeline.h"
#include "graphics/cmd_buffer.h"
#include "render/clear.h"
#include "render/graph/graph_types.h"
#include "container/vector.h"

#include <volk/volk.h>

namespace rnd {

class Graph;

// TODO: Remove PassType. It's a basically deprecated
//       thing and literally all it's ever used for is
//       automatically starting and ending CmdRendering
//       in the render graph. I'm keeping it right now
//       incase I ever need to know what kind of pass
//       it is but realistically I think this will have
//       to go in the future because it's just useless.
enum PassType {
	PassType_Graphics,
	PassType_Compute,
	PassType_Transfer,
	PassType_MaxEnum
};

struct PassTextureEdge {
	GraphTexHandle handle;
	gfx::AccessState state;
	VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;
	gfx::SubresourceRange attachment_range = {};
	GraphTexHandle resolve_handle;
	VkImageLayout resolve_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkResolveModeFlagBits resolve_mode = VK_RESOLVE_MODE_NONE;
	bool should_clear = false;
	Clear clear;
};

struct PassBufferEdge {
	GraphBufHandle handle;
	gfx::AccessState state;
	u64 offset = 0;
	u64 size = 0; // 0 = whole buffer.
};

struct PassContext {
	Graph *graph = nullptr;
	const gfx::CmdBuffer *cmd = nullptr;
	const gfx::RenderInfo *render_info = nullptr;
	const void *user_data = nullptr;
};

typedef void PassRecordFn(const PassContext &ctx);

class Pass {
	friend class Graph;

public:
	Pass();


	/* ==================================================
	   GENERAL
	   ================================================== */

	void SetRecord(PassRecordFn *fn, const void *user_data);
	void SetMultiViewMask(unsigned mask);


	/* ==================================================
	   HELPERS
	   ================================================== */

	GraphTexHandle AddInputTexture(
		GraphTexHandle handle,
		VkPipelineStageFlags2 stage,
		VkAccessFlags2 access
	);

	GraphTexHandle AddOutputTexture(
		GraphTexHandle handle,
		const Clear *clear,
		VkPipelineStageFlags2 stage,
		VkAccessFlags2 access
	);

	GraphBufHandle AddInputBuffer(
		GraphBufHandle handle,
		VkPipelineStageFlags2 stage,
		VkAccessFlags2 access
	);

	GraphBufHandle AddOutputBuffer(
		GraphBufHandle handle,
		VkPipelineStageFlags2 stage,
		VkAccessFlags2 access
	);


	/* ==================================================
	   TEXTURES
	   ================================================== */

	GraphTexHandle WriteColour(GraphTexHandle handle, const Clear *clear);
	GraphTexHandle WriteDepth(GraphTexHandle handle, const Clear *clear);
	GraphTexHandle WriteColourEx(GraphTexHandle handle, const Clear *clear, gfx::SubresourceRange range);
	GraphTexHandle WriteDepthEx(GraphTexHandle handle, const Clear *clear, gfx::SubresourceRange range);
	GraphTexHandle WriteColourResolve(GraphTexHandle msaa, GraphTexHandle resolve, const Clear *clear);
	GraphTexHandle WriteDepthResolve(GraphTexHandle msaa, GraphTexHandle resolve, const Clear *clear);
	GraphTexHandle WriteColourResolveEx(GraphTexHandle msaa, GraphTexHandle resolve, const Clear *clear, gfx::SubresourceRange range);
	GraphTexHandle WriteDepthResolveEx(GraphTexHandle msaa, GraphTexHandle resolve, const Clear *clear, gfx::SubresourceRange range);

	GraphTexHandle ReadTextureGraphics(GraphTexHandle handle);
	GraphTexHandle ReadTextureCompute(GraphTexHandle handle);
	GraphTexHandle WriteTextureCompute(GraphTexHandle handle);

	GraphTexHandle BlitTextureSrc(GraphTexHandle handle);
	GraphTexHandle BlitTextureDst(GraphTexHandle handle);


	/* ==================================================
	   BUFFERS
	   ================================================== */

	GraphBufHandle WriteBufferGraphics(GraphBufHandle handle);
	GraphBufHandle ReadBufferGraphics(GraphBufHandle handle);
	GraphBufHandle WriteBufferCompute(GraphBufHandle handle);
	GraphBufHandle ReadBufferCompute(GraphBufHandle handle);
	GraphBufHandle ReadIndirectBuffer(GraphBufHandle handle);
	GraphBufHandle ClearBuffer(GraphBufHandle handle);

private:
	Graph *graph;

	const char *name;
	PassType type;
	unsigned index;

	bool is_culled;
	PassRecordFn *record;

	const void *user_data;

	unsigned multi_view_mask;

	Vector<PassTextureEdge> input_textures;
	Vector<PassTextureEdge> output_textures;

	Vector<PassBufferEdge> input_buffers;
	Vector<PassBufferEdge> output_buffers;

	// gets filled in by the graph btw
	Vector<VkMemoryBarrier2> memory_barriers;
	Vector<VkImageMemoryBarrier2> texture_barriers;
	Vector<VkBufferMemoryBarrier2> buffer_barriers;
};

}

#endif // RENDER_GRAPH_PASS_H

#ifndef RENDER_GRAPH_GRAPH_H
#define RENDER_GRAPH_GRAPH_H

#include "core/types.h"
#include "core/log.h"
#include "graphics/resource_key.h"
#include "graphics/swapchain.h"
#include "graphics/cmd_buffer.h"
#include "render/buffer_range.h"
#include "render/graph/graph_types.h"
#include "render/graph/pass.h"
#include "render/graph/resource_pool.h"
#include "render/graph/resource_tracker.h"

#include "container/vector.h"

namespace rnd {

/* ==================================================
   VIRTUAL RESOURCES
   ================================================== */

struct GraphTexture {
	gfx::ResourceKey physical_key;
	TextureInfo texture_info;

	unsigned first_pass_index = RENDER_GRAPH_INVALID_INDEX;
	unsigned last_pass_index = RENDER_GRAPH_INVALID_INDEX;
	unsigned ref_count = 0;

	ResourceState state;

	bool is_imported = false;
	gfx::ResourceKey imported_key;
};

struct GraphBuffer {
	gfx::ResourceKey physical_key;
	BufferInfo buffer_info;

	unsigned first_pass_index = RENDER_GRAPH_INVALID_INDEX;
	unsigned last_pass_index = RENDER_GRAPH_INVALID_INDEX;
	unsigned ref_count = 0;

	ResourceState state;

	bool is_imported = false;
	gfx::ResourceKey imported_key;
};


/* ==================================================
   VERSIONING
   ================================================== */

struct GraphTexVersion {
	unsigned resource_index = 0;
	unsigned writer_pass = RENDER_GRAPH_INVALID_INDEX;
	unsigned parent = RENDER_GRAPH_INVALID_INDEX;
};

struct GraphBufVersion {
	unsigned resource_index = 0;
	unsigned writer_pass = RENDER_GRAPH_INVALID_INDEX;
	unsigned parent = RENDER_GRAPH_INVALID_INDEX;
};


/* ==================================================
   IMPORTS
   ================================================== */

struct GraphImportedTexture {
	gfx::ResourceKey external_key;
	GraphTexHandle handle;
};

struct GraphImportedBuffer {
	gfx::ResourceKey external_key;
	GraphBufHandle handle;
};


/* ==================================================
   GRAPH
   ================================================== */

class Graph {
public:
	explicit Graph(LogChannel log_channel);
	~Graph();

	Graph(const Graph &) = delete;
	Graph &operator=(const Graph &) = delete;

	void Reset();

	template <typename T>
	T *NewPassData()
	{
		T *data = new T();
		pass_data.push_back({ data, &DeletePassData<T> });
		return data;
	}


	/* ==================================================
	   BUILDING
	   ================================================== */

	Pass *Add(const char *name, PassType type);

	GraphTexHandle CreateTexture(const TextureInfo &info);
	GraphBufHandle CreateBuffer(const BufferInfo &info);

	GraphTexHandle ImportTexture(gfx::ResourceKey external_key);
	GraphBufHandle ImportBuffer(gfx::ResourceKey external_key);

	GraphTexHandle PushTexVersion(GraphTexHandle parent, unsigned writer_pass_index);
	GraphBufHandle PushBufVersion(GraphBufHandle parent, unsigned writer_pass_index);


	/* ==================================================
	   COMPILATION
	   ================================================== */

	void Compile(const gfx::Swapchain &swapchain);


	/* ==================================================
	   EXECUTION
	   ================================================== */

	void RecordInto(const gfx::CmdBuffer &cmd);
	void PresentToSwapchain(const gfx::Swapchain &swapchain, const gfx::CmdBuffer &cmd);


	/* ==================================================
	   RESOURCE RESOLUTION
	   ================================================== */

	gfx::ResourceKey ResolveTexture(GraphTexHandle handle) const;
	gfx::ResourceKey ResolveTextureView(GraphTexHandle handle, gfx::SubresourceRange range) const;
	gfx::ResourceKey ResolveBuffer(GraphBufHandle handle) const;
	BufferRange ResolveBufferRange(GraphBufHandle handle) const;


	/* ==================================================
	   HELPERS
	   ================================================== */

	void SetBackbuffer(GraphTexHandle handle);
	void SetPresentFilter(VkFilter filter);

	GraphTexHandle GetBackbuffer() const
	{
		return backbuffer_handle;
	}

	gfx::RenderInfo BuildRenderingInfo(const Pass &pass) const;

	GraphTexture *TextureFromHandle(GraphTexHandle handle);
	GraphBuffer *BufferFromHandle(GraphBufHandle handle);

	const GraphTexture *TextureFromHandle(GraphTexHandle handle) const;
	const GraphBuffer *BufferFromHandle(GraphBufHandle handle) const;

	bool TexVersionIsUnwritten(GraphTexHandle handle) const;
	bool BufVersionIsUnwritten(GraphBufHandle handle) const;

	LogChannel GetLogChannel() const
	{
		return log_channel;
	}

private:
	struct PassData {
		void *ptr;
		void (*destroy)(void *ptr);
	};

	template <typename T>
	static void DeletePassData(void *ptr)
	{
		delete static_cast<T *>(ptr);
	}

	void FreePassData();

	void PropagateDependencies();
	void BackpropagateDependencies();
	void AllocateResources(const gfx::Swapchain &swapchain);
	void GenerateBarriers();

	void SyncTextureRead(Pass &pass, const PassTextureEdge &edge);
	void SyncTextureWrite(Pass &pass, const PassTextureEdge &edge);
	void SyncBufferRead(Pass &pass, const PassBufferEdge &edge);
	void SyncBufferWrite(Pass &pass, const PassBufferEdge &edge);

	LogChannel log_channel;

	Vector<Pass> passes;

	ResourcePool pool;
	ResourceTracker tracker;

	GraphTexHandle backbuffer_handle;
	Vector<PassData> pass_data;

	// backbuffer is upscaled/downscaled to the swapchain size when presenting !
	VkFilter present_filter;

	Vector<GraphTexture> texture_resources;
	Vector<GraphTexVersion> texture_versions;

	Vector<GraphBuffer> buffer_resources;
	Vector<GraphBufVersion> buffer_versions;

	Vector<GraphImportedTexture> imported_textures;
	Vector<GraphImportedBuffer> imported_buffers;
};

}

#endif // RENDER_GRAPH_GRAPH_H

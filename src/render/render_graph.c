
internal void
R_GraphInit(R_Graph *graph, Arena *permanent_arena, Arena *frame_arena)
{
	// TODO
}

internal void
R_GraphDestroy(R_Graph *graph)
{
	// TODO
}

internal void
R_GraphReset(R_Graph *graph, const GFX_Device *device)
{
	// TODO
}

internal R_Pass *
R_GraphAdd(R_Graph *graph, String8 name, R_PassType type)
{
	// TODO
}

internal void
R_GraphSetBackbuffer(R_Graph *graph, R_GraphTexHandle handle)
{
	// TODO
}

internal R_GraphTexHandle
R_GraphCreateTexture(R_Graph *graph, const R_TextureInfo *info)
{
	// TODO
}

internal R_GraphBufHandle
R_GraphCreateBuffer(R_Graph *graph, const R_BufferInfo *info)
{
	// TODO
}

internal R_GraphTexHandle
R_GraphImportTexture(R_Graph *graph, const GFX_Device *device, GFX_TextureKey external_key)
{
	// TODO
}

internal R_GraphBufHandle
R_GraphImportBuffer(R_Graph *graph, const GFX_Device *device, GFX_BufferKey external_key)
{
	// TODO
}

internal R_GraphTexHandle
R_GraphPushTexVersion(R_Graph *graph, R_GraphTexHandle parent, u32 write_pass_index)
{
	// TODO
}

internal R_GraphBufHandle
R_GraphPushBufVersion(R_Graph *graph, R_GraphBufHandle parent, u32 write_pass_index)
{
	// TODO
}

internal void
R_GraphCompile(R_Graph *graph, GFX_Device *device, const GFX_Swapchain *swapchain)
{
	// TODO
}

internal void
R_GraphPropogateDependencies(R_Graph *graph)
{
	// TODO
}

internal void
R_GraphBackpropogateDependencies(R_Graph *graph)
{
	// TODO
}

internal void
R_GraphAllocateResources(R_Graph *graph, GFX_Device *device, const GFX_Swapchain *swapchain)
{
	// TODO
}

internal void
R_GraphGenerateBarriers(R_Graph *graph, const GFX_Device *device)
{
	// TODO
}

internal void
R_GraphProcessInvalidateTexture(R_Graph *graph, const GFX_Device *device, R_Pass *pass, const R_PassTextureEdge *edge)
{
	// TODO
}

internal void
R_GraphProcessInvalidateBuffer(R_Graph *graph, const GFX_Device *device, R_Pass *pass, const R_PassBufferEdge *edge)
{
	// TODO
}

internal void
R_GraphProcessFlushTexture(R_Graph *graph, const R_PassTextureEdge *edge)
{
	// TODO
}

internal void
R_GraphProcessFlushBuffer(R_Graph *graph, const R_PassBufferEdge *edge)
{
	// TODO
}

internal void
R_GraphExecute(R_Graph *graph,
			   GFX_Device *device,
			   const GFX_Swapchain *swapchain,
			   const GFX_CmdBuffer *cmd,
			   const R_Scene *scene,
			   const R_Camera *camera,
			   f32 delta_time, f32 elapsed_time)
{
	// TODO
}

internal void
R_GraphPresentToSwapchain(R_Graph *graph,
						  const GFX_Device *device,
						  const GFX_Swapchain *swapchain,
						  const GFX_CmdBuffer *cmd)
{
	// TODO
}

internal const GFX_Texture *
R_GraphResolveTexture(const R_Graph *graph,
					  const GFX_Device *device,
					  R_GraphTexHandle handle)
{
	// TODO
}

internal const GFX_TextureView *
R_GraphResolveTextureView(const R_Graph *graph,
						  const GFX_Device *device,
						  R_GraphTexHandle key,
						  GFX_SubresourceRange range)
{
	// TODO
}

internal const GFX_Buffer *
R_GraphResolveBuffer(const R_Graph *graph,
					 const GFX_Device *device,
					 R_GraphBufHandle key)
{
	// TODO
}

internal GFX_BufferRange
R_GraphResolveBufferRange(const R_Graph *graph,
						  const GFX_Device *device,
						  R_GraphBufHandle key)
{
	// TODO
}

internal GFX_RenderInfo
R_GraphBuildRenderingInfo(const R_Graph *graph, const GFX_Device *device, const R_Pass *pass)
{
	// TODO
}

internal R_GraphTexture *
R_GraphTextureFromHandle(R_Graph *graph, R_GraphTexHandle handle)
{
	// TODO
}

internal R_GraphBuffer *
R_GraphBufferFromHandle(R_Graph *graph, R_GraphBufHandle handle)
{
	// TODO
}

internal b32
R_GraphTexVersionIsUnwritten(const R_Graph *graph, R_GraphTexHandle handle)
{
	// TODO
}

internal b32
R_GraphBufVersionIsUnwritten(const R_Graph *graph, R_GraphBufHandle handle)
{
	// TODO
}

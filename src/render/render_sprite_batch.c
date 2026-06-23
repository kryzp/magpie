
internal void
R_SpriteBatchCreateQuad(R_SpriteBatch *b)
{
	R_SpriteBatchVertex vertices[4] = {
		{ v2(0.f, 0.f), v2(0.f, 0.f), v4(1.f, 1.f, 1.f, 1.f), v4(0.f, 0.f, 0.f, 0.f) },
		{ v2(1.f, 0.f), v2(1.f, 0.f), v4(1.f, 1.f, 1.f, 1.f), v4(0.f, 0.f, 0.f, 0.f) },
		{ v2(0.f, 1.f), v2(0.f, 1.f), v4(1.f, 1.f, 1.f, 1.f), v4(0.f, 0.f, 0.f, 0.f) },
		{ v2(1.f, 1.f), v2(1.f, 1.f), v4(1.f, 1.f, 1.f, 1.f), v4(0.f, 0.f, 0.f, 0.f) }
	};
	
	u16 indices[6] = {
		0, 2, 1,
		2, 3, 1
	};

	R_MeshAlloc(&b->quad, b->device,
				sizeof(R_SpriteBatchVertex), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));
}

internal void
R_SpriteBatchInit(R_SpriteBatch *b, G_Device *device, LOG_Channel log_channel)
{
	b->device = device;
	b->log_channel = log_channel;

	R_SpriteBatchCreateQuad(b);
	// TODO
}

internal void
R_SpriteBatchDestroy(R_SpriteBatch *b)
{
	R_MeshDestroy(&b->quad, b->device);
}

internal void
R_SpriteBatchBegin(R_SpriteBatch *b)
{
	b->task_count = 0;
}

internal void
R_SpriteBatchEnd(R_SpriteBatch *b, R_Graph *g)
{
	for (u32 i = 0; i < b->task_count; i++)
	{
		R_SpriteBatchTask *task = &b->tasks[i];
	}
}

internal void
R_SpriteBatchClear(R_SpriteBatch *b)
{
	b->task_count = 0;
}

internal void
R_SpriteBatchSetPixelSnap(R_SpriteBatch *b, b32 v)
{
	b->pixel_snap = v;
}

#define SbParam(type, lower, upper)										\
	internal void R_SpriteBatchPush##upper(R_SpriteBatch *b, type v)	\
	{																	\
		DebugLogAssert(b->log_channel, b->lower##_stack_count < ArraySize(b->lower##_stack), "Exceeded maximum parameter stack size for " #lower  " stack."); \
		b->lower##_stack[b->lower##_stack_count] = v;					\
		b->lower##_stack_count++;										\
	}																	\
	internal type R_SpriteBatchPop##upper(R_SpriteBatch *b)				\
	{																	\
		DebugLogAssert(b->log_channel, b->lower##_stack_count > 0, "Parameter stack size for " #lower " stack must be greater than zero."); \
		b->lower##_stack_count--;										\
		return b->lower##_stack[b->lower##_stack_count];				\
	}																	\
	internal type R_SpriteBatchPeek##upper(const R_SpriteBatch *b)		\
	{																	\
		DebugLogAssert(b->log_channel, b->lower##_stack_count > 0, "Parameter stack size for " #lower " stack must be greater than zero."); \
		return b->lower##_stack[b->lower##_stack_count - 1];			\
	}
#include "render_sprite_batch_params.inc"
#undef SbParam

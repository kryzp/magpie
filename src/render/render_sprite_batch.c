
internal void
R_SpriteBatchInit(R_SpriteBatch *b, LOG_Channel log_channel)
{
	MemZeroStruct(b);
	
	b->log_channel = log_channel;

	// TODO
}

internal void
R_SpriteBatchBegin(R_SpriteBatch *b)
{
	// TODO
}

internal void
R_SpriteBatchEnd(R_SpriteBatch *b, R_Graph *graph)
{
	// TODO
}

internal void
R_SpriteBatchClear(R_SpriteBatch *b)
{
	// TODO
}

internal void
R_SpriteBatchSetPixelSnap(R_SpriteBatch *b, b32 b)
{
	b->pixel_snap = b;
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
		DebugLogAssert(b->log_channel, b->lower_##stack_count > 0, "Parameter stack size for " #lower " stack must be greater than zero."); \
		b->lower##_stack_count--;										\
		return b->lower##_stack[b->lower##_stack_count];				\
	}																	\
	internal type R_SpriteBatchPeek##upper(const R_SpriteBatch *b)		\
	{																	\
		DebugLogAssert(b->log_channel, b->lower_##stack_count > 0, "Parameter stack size for " #lower " stack must be greater than zero."); \
		return b->lower##_stack[b->lower##_stack_count - 1];			\
	}
#include "render_sprite_batch_params.inc"
#undef SbParam

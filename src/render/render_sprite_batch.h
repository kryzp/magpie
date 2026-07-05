#ifndef RENDER_SPRITE_BATCH_H
#define RENDER_SPRITE_BATCH_H

/*
 * Essentially this is a utility for rendering any kind of 2D stuff.
 * Based off of the XNA/FNA/MonoGame SpriteBatch.
 * porting my sprite batch implementation from my literal almost
 * decade old 2d game framework project :p
 * https://github.com/kryzp/leviathan/blob/master/public/lev/graphics/sprite_batch.h
 * ...crazy.
 */

#define R_SPRITE_BATCH_MAX_TASKS 512
#define R_SPRITE_BATCH_STACK_MEMORY 128

typedef struct R_SpriteBatchVertex R_SpriteBatchVertex;
struct R_SpriteBatchVertex
{
	v2 pos;
	v2 uv;
	v4 col;
	v4 mode;
};

typedef enum R_SpriteSort
{
	R_SpriteSort_None,
	R_SpriteSort_FrontToBack,
	R_SpriteSort_BackToFront,
	R_SpriteSort_Deferred,
	R_SpriteSort_COUNT
}
R_SpriteSort;

typedef struct R_SpriteBatchTask R_SpriteBatchTask;
struct R_SpriteBatchTask
{
	G_TextureKey texture_key;
	G_SamplerKey sampler_key;

	f32 layer;

	u32 vertex_count;
	R_SpriteBatchVertex *vertices;

	u32 index_count;
	u32 *indices;
};

typedef struct R_SpriteBatch R_SpriteBatch;
struct R_SpriteBatch
{
	LOG_Channel log_channel;
	
	u32 task_count;
	R_SpriteBatchTask tasks[R_SPRITE_BATCH_MAX_TASKS];

	R_Mesh quad;

	// snap all draw coords to integers.
	b32 pixel_snap;
	
#define SbParam(type, lower, upper)						\
	type lower##_stack[R_SPRITE_BATCH_STACK_MEMORY];	\
	u32 lower##_stack_count;
#include "render_sprite_batch_params.inc"
#undef SbParam
};

static void R_SpriteBatchInit(R_SpriteBatch *b, LOG_Channel log_channel);
static void R_SpriteBatchDestroy(R_SpriteBatch *b);

static void R_SpriteBatchBegin(R_SpriteBatch *b);
static void R_SpriteBatchEnd(R_SpriteBatch *b, R_Graph *g);

static void R_SpriteBatchClear(R_SpriteBatch *b);

static void R_SpriteBatchSetPixelSnap(R_SpriteBatch *b, b32 v);

static void R_SpriteBatchRect(R_SpriteBatch *b, v2 pos, v2 size);

#define SbParam(type, lower, upper)										\
	static void R_SpriteBatchPush##upper(R_SpriteBatch *b, type v);	\
	static type R_SpriteBatchPop##upper(R_SpriteBatch *b);			\
	static type R_SpriteBatchPeek##upper(const R_SpriteBatch *b);
#include "render_sprite_batch_params.inc"
#undef SbParam

#endif // RENDER_SPRITE_BATCH_H

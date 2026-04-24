#ifndef ASSET_HANDLE_H
#define ASSET_HANDLE_H

#define AST_HANDLE_INVALID_INDEX ((u32)-1)

typedef struct AST_Handle AST_Handle;
struct AST_Handle
{
	u32 index;
	u32 generation;
};

internal inline AST_Handle
AST_HandleNull(void)
{
	AST_Handle handle = {0};
	handle.index = AST_HANDLE_INVALID_INDEX;
	handle.generation = 0;

	return handle;
}

internal inline b32
AST_HandleMatch(AST_Handle a, AST_Handle b)
{
	return (a.index == b.index &&
			a.generation == b.generation);
}

#endif // ASSET_HANDLE_H

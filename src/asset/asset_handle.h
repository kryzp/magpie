#ifndef ASSET_HANDLE_H
#define ASSET_HANDLE_H

#define A_HANDLE_INVALID_INDEX ((u32)-1)

typedef struct A_Handle A_Handle;
struct A_Handle
{
	u32 index;
	u32 generation;

	A_Type type;
};

internal inline A_Handle
A_HandleNull(void)
{
	A_Handle handle = {0};
	handle.index = A_HANDLE_INVALID_INDEX;
	handle.generation = 0;

	return handle;
}

internal inline b32
A_HandleMatch(A_Handle a, A_Handle b)
{
	return (a.index == b.index &&
			a.generation == b.generation);
}

#endif // ASSET_HANDLE_H

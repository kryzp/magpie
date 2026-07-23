#ifndef SCRIPT_HANDLE_H
#define SCRIPT_HANDLE_H

typedef struct S_Handle S_Handle;
struct S_Handle
{
	u32 index;
	u32 generation;
};

internal inline S_Handle S_HandleNull(void)
{
	S_Handle handle = {0};
	return handle;
}

internal inline b32 S_HandleIsNull(S_Handle handle)
{
	return handle.index == 0 && handle.generation == 0;
}

internal inline b32 S_HandleMatch(S_Handle a, S_Handle b)
{
	return (a.index == b.index &&
			a.generation == b.generation);
}

typedef struct S_Ref S_Ref;
struct S_Ref
{
	i32 value;
};

internal inline S_Ref S_RefNull(void)
{
	S_Ref ref = {0};
	return ref;
}

internal inline b32 S_RefIsNull(S_Ref ref)
{
	return ref.value < 0;
}

internal inline b32 S_RefMatch(S_Ref a, S_Ref b)
{
	return a.value == b.value;
}

#endif // SCRIPT_HANDLE_H

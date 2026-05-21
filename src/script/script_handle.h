#ifndef SCRIPT_HANDLE_H
#define SCRIPT_HANDLE_H

typedef struct SCR_Handle SCR_Handle;
struct SCR_Handle
{
	u32 index;
	u32 generation;
};

internal inline b32
SCR_HandleIsNull(SCR_Handle handle)
{
	return handle.index == 0 && handle.generation == 0;
}

typedef struct SCR_ScriptRef SCR_ScriptRef;
struct SCR_ScriptRef
{
	i32 value;
};

internal inline b32
SCR_ScriptRefIsNull(SCR_ScriptRef ref)
{
	return ref.value < 0;
}

#endif // SCRIPT_HANDLE_H

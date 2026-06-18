#ifndef SCRIPT_HANDLE_H
#define SCRIPT_HANDLE_H

typedef struct SCR_Handle SCR_Handle;
struct SCR_Handle
{
	u32 index;
	u32 generation;
};

internal inline SCR_Handle
SCR_HandleNull(void)
{
	SCR_Handle handle = {0};
	return handle;
}

internal inline b32
SCR_HandleIsNull(SCR_Handle handle)
{
	return handle.index == 0 && handle.generation == 0;
}

internal inline b32
SCR_HandleMatch(SCR_Handle a, SCR_Handle b)
{
	return (a.index == b.index &&
			a.generation == b.generation);
}

typedef struct SCR_ScriptRef SCR_ScriptRef;
struct SCR_ScriptRef
{
	i32 value;
};

internal inline SCR_ScriptRef
SCR_ScriptRefNull(void)
{
	SCR_ScriptRef ref = {0};
	return ref;
}

internal inline b32
SCR_ScriptRefIsNull(SCR_ScriptRef ref)
{
	return ref.value < 0;
}

internal inline b32
SCR_ScriptRefMatch(SCR_ScriptRef a, SCR_ScriptRef b)
{
	return a.value == b.value;
}

#endif // SCRIPT_HANDLE_H

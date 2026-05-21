#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

typedef struct SCR_System SCR_System;
struct SCR_System
{
	u32 todo_this_is_an_incomplete_thing;
};

internal SCR_System *SCR_GetSystem(SCR_Context *ctx);

internal void SCR_Init(SCR_System *system, Arena *arena, LOG_Channel log_channel);
internal void SCR_Destroy(SCR_System *system);
internal void SCR_Tick(SCR_System *system, f32 dt);

internal void SCR_BindGlobal(SCR_System *system, String8 name, SCR_BindingFn *fn);
internal void SCR_BindToTable(SCR_System *system, String8 table, String8 name, SCR_BindingFn *fn);

internal void SCR_BindGlobalWithUpval(SCR_System *system, String8 name, SCR_BindingFn *fn, void * const *upvalues, u32 n);
internal void SRC_BindToTableWithUpval(SCR_System *system, String8 table, String8 name, SCR_BindingFn *fn, void * const *upvalues, u32 n);

internal SCR_ScriptRef SCR_Compile(SCR_System *system, String8 source, String8 chunk_name);
internal void SCR_Release(SCR_System *system, SCR_ScriptRef ref);

internal void SCR_Stop(SCR_System *system, SCR_Handle handle);
internal b32 SCR_IsRunning(const SCR_System *system, SCR_Handle handle);

internal void SCR_SetOnFinish(SCR_System *system, SCR_Handle handle, SCR_FinishFn *fn, void *user_data);

internal SCR_Handle SCR_Play(SCR_System *system, SCR_ScriptRef ref);
internal SCR_Handle SCR_PlayEx(SCR_System *system, SCR_ScriptRef ref, const SCR_Argument *args, u32 arg_count);

internal void SCR_DoFile(SCR_System *system, String8 path); // quick and dirty synchronous load-in-a-file-and-also-execute-it-at-once

#endif // SCRIPT_SYSTEM_H

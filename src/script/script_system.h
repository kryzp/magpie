#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#define S_MAX_COROUTINES      512
#define S_MAX_PENDING_SIGNALS 512

typedef struct S_System S_System;
typedef struct S_Context S_Context;
typedef struct S_Coroutine S_Coroutine;

#define S_BINDING_SIG(fn) void fn(S_Context *ctx)
#define S_BINDING_DEF(fn) internal S_BINDING_SIG(fn)

#define S_CONTINUE_SIG(fn) void fn(S_Context *ctx, void *user_data)
#define S_CONTINUE_DEF(fn) internal S_CONTINUE_SIG(fn)

#define S_FINISH_SIG(fn) void fn(S_System *system, S_Coroutine *co, void *user_data)
#define S_FINISH_DEF(fn) internal S_FINISH_SIG(fn)

typedef S_BINDING_SIG(S_BindingFn);
typedef S_CONTINUE_SIG(S_ContinueFn);
typedef S_FINISH_SIG(S_FinishFn);

internal S_System *S_Init(Arena *arena, LOG_Channel log_channel);
internal void S_Destroy(S_System *system);
internal void S_Tick(S_System *system, f32 dt);

internal S_Ref S_Compile(S_System *system, IO_ByteSpan source, String8 chunk_name);
internal void S_Release(S_System *system, S_Ref ref);

internal S_Ref S_ExecuteModule(S_System *system, S_Ref chunk_ref);
internal S_Ref S_NewInstance(S_System *system, S_Ref module_ref);

internal S_Handle S_CallMethod(S_System *system, S_Ref instance_ref, String8 method_name);
internal S_Handle S_CallMethodEx(S_System *system, S_Ref instance_ref, String8 method_name, const S_Argument *args, u32 arg_count);

internal void S_Stop(S_System *system, S_Handle handle);

internal b32 S_IsRunning(const S_System *system, S_Handle handle);

internal void S_FireSignal(S_System *system, String8 name);

internal void S_SetOnFinish(S_System *system, S_Handle handle, S_FinishFn *fn, void *user_data);

internal void S_BindGlobal(S_System *system, String8 name, S_BindingFn *fn);
internal void S_BindToTable(S_System *system, String8 table, String8 name, S_BindingFn *fn);
internal void S_BindGlobalEx(S_System *system, String8 name, S_BindingFn *fn, void * const *upvalues, u32 n);
internal void S_BindToTableEx(S_System *system, String8 table, String8 name, S_BindingFn *fn, void * const *upvalues, u32 n);

#endif // SCRIPT_SYSTEM_H

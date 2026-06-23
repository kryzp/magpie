#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#define S_MAX_COROUTINES      512
#define S_MAX_PENDING_SIGNALS 512

typedef struct S_System S_System;
typedef struct S_Context S_Context;
typedef struct S_Coroutine S_Coroutine;

#define S_BINDING_DEF(fn) void fn(S_Context *ctx)
#define S_CONTINUE_DEF(fn) void fn(S_Context *ctx, void *user_data)
#define S_FINISH_DEF(fn) void fn(S_System *system, S_Coroutine *co, void *user_data)

typedef S_BINDING_DEF(S_BindingFn);
typedef S_CONTINUE_DEF(S_ContinueFn);
typedef S_FINISH_DEF(S_FinishFn);

static S_System *S_Init(Arena *arena, LOG_Channel log_channel);
static void S_Destroy(S_System *system);
static void S_Tick(S_System *system, f32 dt);

static S_Ref S_Compile(S_System *system, IO_ByteSpan source, String8 chunk_name);
static void S_Release(S_System *system, S_Ref ref);

static S_Ref S_ExecuteModule(S_System *system, S_Ref chunk_ref);
static S_Ref S_NewInstance(S_System *system, S_Ref module_ref);

static S_Handle S_CallMethod(S_System *system, S_Ref instance_ref, String8 method_name);
static S_Handle S_CallMethodEx(S_System *system, S_Ref instance_ref, String8 method_name, const S_Argument *args, u32 arg_count);

static void S_Stop(S_System *system, S_Handle handle);

static b32 S_IsRunning(const S_System *system, S_Handle handle);

static void S_FireSignal(S_System *system, String8 name);

static void S_SetOnFinish(S_System *system, S_Handle handle, S_FinishFn *fn, void *user_data);

static void S_BindGlobal(S_System *system, String8 name, S_BindingFn *fn);
static void S_BindToTable(S_System *system, String8 table, String8 name, S_BindingFn *fn);
static void S_BindGlobalEx(S_System *system, String8 name, S_BindingFn *fn, void *const *upvalues, u32 n);
static void S_BindToTableEx(S_System *system, String8 table, String8 name, S_BindingFn *fn, void *const *upvalues, u32 n);

#endif // SCRIPT_SYSTEM_H

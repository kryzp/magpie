#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#define S_MAX_COROUTINES      512
#define S_MAX_PENDING_SIGNALS 512

typedef struct S_System S_System;
typedef struct S_Context S_Context;
typedef struct S_Coroutine S_Coroutine;

#define S_BINDING_DEF(fn) void fn(S_Context *ctx)
#define S_CONTINUE_DEF(fn) void fn(S_Context *ctx, void *user_data)
#define S_FINISH_DEF(fn) void fn(S_Coroutine *co, void *user_data)

typedef S_BINDING_DEF(S_BindingFn);
typedef S_CONTINUE_DEF(S_ContinueFn);
typedef S_FINISH_DEF(S_FinishFn);

internal S_System *S_AllocAndSelect(Arena *arena, LOG_Channel log_channel);
internal void S_Destroy(void);
internal void S_SelectContext(S_System *system);
internal void S_Tick(f32 dt);

internal S_Ref S_Compile(IO_ByteSpan source, String8 chunk_name);
internal void S_Release(S_Ref ref);

internal S_Ref S_ExecuteModule(S_Ref chunk_ref);
internal S_Ref S_NewInstance(S_Ref module_ref);

internal S_Handle S_CallMethod(S_Ref instance_ref, String8 method_name);
internal S_Handle S_CallMethodEx(S_Ref instance_ref, String8 method_name, const S_Argument *args, u32 arg_count);

internal void S_Stop(S_Handle handle);

internal b32 S_IsRunning(S_Handle handle);

internal void S_FireSignal(String8 name);

internal void S_SetOnFinish(S_Handle handle, S_FinishFn *fn, void *user_data);

internal void S_BindGlobal(String8 name, S_BindingFn *fn);
internal void S_BindToTable(String8 table, String8 name, S_BindingFn *fn);
internal void S_BindGlobalEx(String8 name, S_BindingFn *fn, void *const *upvalues, u32 n);
internal void S_BindToTableEx(String8 table, String8 name, S_BindingFn *fn, void *const *upvalues, u32 n);

internal u32             S_CtxGetArgCount(S_Context *ctx);

internal f32             S_CtxGetArgF32(S_Context *ctx, u32 idx);
internal i32             S_CtxGetArgI32(S_Context *ctx, u32 idx);
internal b32             S_CtxGetArgBool(S_Context *ctx, u32 idx);
internal String8         S_CtxGetArgStr(S_Context *ctx, u32 idx);
internal u32             S_CtxGetArgTaggedU32(S_Context *ctx, u32 idx, u32 expected_tag);

internal f32             S_CtxGetArgF32Opt(S_Context *ctx, u32 idx, f32 fallback);
internal i32             S_CtxGetArgI32Opt(S_Context *ctx, u32 idx, i32 fallback);
internal b32             S_CtxGetArgB32Opt(S_Context *ctx, u32 idx, b32 fallback);
internal String8         S_CtxGetArgStrOpt(S_Context *ctx, u32 idx, String8 fallback);

internal void           *S_CtxUpvaluePtr(S_Context *ctx, u32 idx);

internal void            S_CtxReturnNil(S_Context *ctx);
internal void            S_CtxReturnF32(S_Context *ctx, f32 v);
internal void            S_CtxReturnI32(S_Context *ctx, i32 v);
internal void            S_CtxReturnB32(S_Context *ctx, b32 v);
internal void            S_CtxReturnString8(S_Context *ctx, String8 v);
internal void            S_CtxReturnTaggedU32(S_Context *ctx, u32 v, u32 type_tag);

internal void            S_CtxYield(S_Context *ctx);
internal void            S_CtxYieldTime(S_Context *ctx, f32 time_s);
internal void            S_CtxYieldSignal(S_Context *ctx, String8 name);
internal void            S_CtxYieldSignalCont(S_Context *ctx, String8 name, S_ContinueFn *cont, void *user_data);

internal void            S_CtxThrow(S_Context *ctx, const char *fmt, ...);

#endif // SCRIPT_SYSTEM_H

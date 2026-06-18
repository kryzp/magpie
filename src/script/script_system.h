#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#define SCR_MAX_COROUTINES      512
#define SCR_MAX_PENDING_SIGNALS 512

typedef struct SCR_System SCR_System;
typedef struct SCR_Context SCR_Context;
typedef struct SCR_Coroutine SCR_Coroutine;

#define SCR_BINDING_SIG(fn) void fn(SCR_Context *ctx)
#define SCR_BINDING_DEF(fn) internal SCR_BINDING_SIG(fn)

#define SCR_CONTINUE_SIG(fn) void fn(SCR_Context *ctx, void *user_data)
#define SCR_CONTINUE_DEF(fn) internal SCR_CONTINUE_SIG(fn)

#define SCR_FINISH_SIG(fn) void fn(SCR_System *system, SCR_Coroutine *co, void *user_data)
#define SCR_FINISH_DEF(fn) internal SCR_FINISH_SIG(fn)

typedef SCR_BINDING_SIG(SCR_BindingFn);
typedef SCR_CONTINUE_SIG(SCR_ContinueFn);
typedef SCR_FINISH_SIG(SCR_FinishFn);

internal SCR_System *SCR_Init(Arena *arena, LOG_Channel log_channel);
internal void        SCR_Destroy(SCR_System *system);
internal void        SCR_Tick(SCR_System *system, f32 dt);

internal void SCR_BindGlobal(SCR_System *system, String8 name, SCR_BindingFn *fn);
internal void SCR_BindToTable(SCR_System *system, String8 table, String8 name, SCR_BindingFn *fn);
internal void SCR_BindGlobalWithUpval(SCR_System *system, String8 name, SCR_BindingFn *fn, void * const *upvalues, u32 n);
internal void SCR_BindToTableWithUpval(SCR_System *system, String8 table, String8 name, SCR_BindingFn *fn, void * const *upvalues, u32 n);

internal SCR_ScriptRef SCR_Compile(SCR_System *system, IO_ByteSpan source, String8 chunk_name);
internal void SCR_Release(SCR_System *system, SCR_ScriptRef ref);

internal SCR_ScriptRef SCR_ExecuteModule(SCR_System *system, SCR_ScriptRef chunk_ref);
internal SCR_ScriptRef SCR_NewInstance(SCR_System *system, SCR_ScriptRef module_ref);

internal SCR_Handle SCR_CallMethod(SCR_System *system, SCR_ScriptRef instance_ref, String8 method_name);
internal SCR_Handle SCR_CallMethodEx(SCR_System *system, SCR_ScriptRef instance_ref, String8 method_name, const SCR_Argument *args, u32 arg_count);

internal void SCR_Stop(SCR_System *system, SCR_Handle handle);

internal b32 SCR_IsRunning(const SCR_System *system, SCR_Handle handle);

internal void SCR_SetOnFinish(SCR_System *system, SCR_Handle handle, SCR_FinishFn *fn, void *user_data);

internal void SCR_FireSignal(SCR_System *system, String8 name);

internal u32     SCR_GetArgCount     (SCR_Context *ctx);

internal f32     SCR_ArgF32          (SCR_Context *ctx, u32 idx);
internal i32     SCR_ArgI32          (SCR_Context *ctx, u32 idx);
internal b32     SCR_ArgBool         (SCR_Context *ctx, u32 idx);
internal String8 SCR_ArgString       (SCR_Context *ctx, u32 idx);
internal u32     SCR_ArgTaggedU32    (SCR_Context *ctx, u32 idx, u32 expected_tag);

internal f32     SCR_ArgF32Opt       (SCR_Context *ctx, u32 idx, f32 fallback);
internal i32     SCR_ArgI32Opt       (SCR_Context *ctx, u32 idx, i32 fallback);
internal b32     SCR_ArgB32Opt       (SCR_Context *ctx, u32 idx, b32 fallback);
internal String8 SCR_ArgStringOpt    (SCR_Context *ctx, u32 idx, String8 fallback);

internal void   *SCR_UpvaluePtr      (SCR_Context *ctx, u32 idx);

internal void    SCR_ReturnNil       (SCR_Context *ctx);
internal void    SCR_ReturnF32       (SCR_Context *ctx, f32 v);
internal void    SCR_ReturnI32       (SCR_Context *ctx, i32 v);
internal void    SCR_ReturnB32       (SCR_Context *ctx, b32 v);
internal void    SCR_ReturnString8   (SCR_Context *ctx, String8 v);
internal void    SCR_ReturnTaggedU32 (SCR_Context *ctx, u32 v, u32 type_tag);

internal void    SCR_Yield           (SCR_Context *ctx);
internal void    SCR_YieldTime       (SCR_Context *ctx, f32 time_s);
internal void    SCR_YieldSignal     (SCR_Context *ctx, String8 name);
internal void    SCR_YieldSignalCont (SCR_Context *ctx, String8 name, SCR_ContinueFn *cont, void *user_data);

internal void    SCR_Throw(SCR_Context *ctx, const char *fmt, ...);

#endif // SCRIPT_SYSTEM_H

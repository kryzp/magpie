#ifndef SCRIPT_BINDING_H
#define SCRIPT_BINDING_H

// I'm trying to keep things at least a little
// self-contained so all lua based stuff
// goes into the source files.
// so these are some opaque types.

typedef struct SCR_Context SCR_Context;
typedef struct SCR_Coroutine SCR_Coroutine;
typedef struct SCR_System SCR_System;

#define SCR_BINDING_SIG(fn) void fn(SCR_Context *ctx);
#define SCR_BINDING_DEF(fn) internal SCR_BINDING_SIG(fn)

#define SCR_CONTINUE_SIG(fn) void fn(SCR_Context *ctx, void *user_data);
#define SCR_CONTINUE_DEF(fn) internal SCR_CONTINUE_SIG(fn)

#define SCR_FINISH_SIG(fn) void fn(SCR_System *system, SCR_Coroutine *co, void *user_data);
#define SCR_FINISH_DEF(fn) internal SCR_FINISH_SIG(fn)

typedef SCR_BINDING_SIG(SCR_BindingFn); // starts the script
typedef SCR_CONTINUE_SIG(SCR_ContinueFn); // when a yielded coroutine wakes up
typedef SCR_FINISH_SIG(SCR_FinishFn); // when needing to cleanup / release resources after a coroutine

internal u32 SCR_GetArgCount(SCR_Context *ctx);
internal f32 SCR_ArgF32(SCR_Context *ctx, u32 idx);
internal i32 SCR_ArgI32(SCR_Context *ctx, u32 idx);
internal b32 SCR_ArgBool(SCR_Context *ctx, u32 idx);
internal String8 SCR_ArgString(SCR_Context *ctx, u32 idx);
internal u32 SCR_ArgTaggedU32(SCR_Context *ctx, u32 idx, u32 exp_tag);
internal u32 SCR_ArgCount(SCR_Context *ctx, u32 idx);

internal f32 SCR_ArgF32Opt(SCR_Context *ctx, u32 idx, f32 fallback);
internal i32 SCR_ArgI32Opt(SCR_Context *ctx, u32 idx, i32 fallback);
internal b32 SCR_ArgB32Opt(SCR_Context *ctx, u32 idx, b32 fallback);
internal String8 SCR_ArgStringOpt(SCR_Context *ctx, u32 idx, String8 fallback);

internal void *SCR_UpvaluePtr(SCR_Context *ctx, u32 idx);

internal void SCR_ReturnNil(SCR_Context *ctx);
internal void SCR_ReturnF32(SCR_Context *ctx, f32 v);
internal void SCR_ReturnI32(SCR_Context *ctx, i32 v);
internal void SCR_ReturnB32(SCR_Context *ctx, b32 v);
internal void SCR_ReturnString8(SCR_Context *ctx, String8 s);
internal void SCR_ReturnTaggedU32(SCR_Context *ctx, u32 v, u32 type_tag);

internal void SCR_Yield(SCR_Context *ctx);
internal void SCR_YieldTime(SCR_Context *ctx, f32 time_s);
internal void SCR_YieldSignal(SCR_Context *ctx, String8 name);
internal void SCR_YieldSignalCont(SCR_Context *ctx, String8 name, SCR_ContinueFn *cont, void *user_data);

internal void SCR_Throw(SCR_Context *ctx, const char *fmt, ...);

#endif // SCRIPT_BINDING_H

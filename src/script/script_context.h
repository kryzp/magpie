#ifndef SCRIPT_CONTEXT_H
#define SCRIPT_CONTEXT_H

internal u32             S_CtxGetArgCount              (S_Context *ctx);

internal f32             S_CtxGetArgF32                (S_Context *ctx, u32 idx);
internal i32             S_CtxGetArgI32                (S_Context *ctx, u32 idx);
internal b32             S_CtxGetArgBool               (S_Context *ctx, u32 idx);
internal String8         S_CtxGetArgStr                (S_Context *ctx, u32 idx);
internal u32             S_CtxGetArgTaggedU32          (S_Context *ctx, u32 idx, u32 expected_tag);

internal f32             S_CtxGetArgF32Opt             (S_Context *ctx, u32 idx, f32 fallback);
internal i32             S_CtxGetArgI32Opt             (S_Context *ctx, u32 idx, i32 fallback);
internal b32             S_CtxGetArgB32Opt             (S_Context *ctx, u32 idx, b32 fallback);
internal String8         S_CtxGetArgStrOpt             (S_Context *ctx, u32 idx, String8 fallback);

internal void           *S_CtxUpvaluePtr               (S_Context *ctx, u32 idx);

internal void            S_CtxReturnNil                (S_Context *ctx);
internal void            S_CtxReturnF32                (S_Context *ctx, f32 v);
internal void            S_CtxReturnI32                (S_Context *ctx, i32 v);
internal void            S_CtxReturnB32                (S_Context *ctx, b32 v);
internal void            S_CtxReturnString8            (S_Context *ctx, String8 v);
internal void            S_CtxReturnTaggedU32          (S_Context *ctx, u32 v, u32 type_tag);

internal void            S_CtxYield                    (S_Context *ctx);
internal void            S_CtxYieldTime                (S_Context *ctx, f32 time_s);
internal void            S_CtxYieldSignal              (S_Context *ctx, String8 name);
internal void            S_CtxYieldSignalCont          (S_Context *ctx, String8 name, S_ContinueFn *cont, void *user_data);

internal void            S_CtxThrow                    (S_Context *ctx, const char *fmt, ...);

#endif // SCRIPT_CONTEXT_H

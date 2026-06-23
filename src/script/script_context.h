#ifndef SCRIPT_CONTEXT_H
#define SCRIPT_CONTEXT_H

static u32             S_CtxGetArgCount              (S_Context *ctx);

static f32             S_CtxGetArgF32                (S_Context *ctx, u32 idx);
static i32             S_CtxGetArgI32                (S_Context *ctx, u32 idx);
static b32             S_CtxGetArgBool               (S_Context *ctx, u32 idx);
static String8         S_CtxGetArgStr                (S_Context *ctx, u32 idx);
static u32             S_CtxGetArgTaggedU32          (S_Context *ctx, u32 idx, u32 expected_tag);

static f32             S_CtxGetArgF32Opt             (S_Context *ctx, u32 idx, f32 fallback);
static i32             S_CtxGetArgI32Opt             (S_Context *ctx, u32 idx, i32 fallback);
static b32             S_CtxGetArgB32Opt             (S_Context *ctx, u32 idx, b32 fallback);
static String8         S_CtxGetArgStrOpt             (S_Context *ctx, u32 idx, String8 fallback);

static void           *S_CtxUpvaluePtr               (S_Context *ctx, u32 idx);

static void            S_CtxReturnNil                (S_Context *ctx);
static void            S_CtxReturnF32                (S_Context *ctx, f32 v);
static void            S_CtxReturnI32                (S_Context *ctx, i32 v);
static void            S_CtxReturnB32                (S_Context *ctx, b32 v);
static void            S_CtxReturnString8            (S_Context *ctx, String8 v);
static void            S_CtxReturnTaggedU32          (S_Context *ctx, u32 v, u32 type_tag);

static void            S_CtxYield                    (S_Context *ctx);
static void            S_CtxYieldTime                (S_Context *ctx, f32 time_s);
static void            S_CtxYieldSignal              (S_Context *ctx, String8 name);
static void            S_CtxYieldSignalCont          (S_Context *ctx, String8 name, S_ContinueFn *cont, void *user_data);

static void            S_CtxThrow                    (S_Context *ctx, const char *fmt, ...);

#endif // SCRIPT_CONTEXT_H

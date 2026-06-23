
static u32 S_CtxGetArgCount(S_Context *ctx)
{
	return lua_gettop(ctx->lua);
}

static f32 S_CtxGetArgF32(S_Context *ctx, u32 idx)
{
	return luaL_checknumber(ctx->lua, idx + 1);
}

static i32 S_CtxGetArgI32(S_Context *ctx, u32 idx)
{
	return luaL_checkinteger(ctx->lua, idx + 1);
}

static b32 S_CtxGetArgBool(S_Context *ctx, u32 idx)
{
	luaL_checktype(ctx->lua, idx + 1, LUA_TBOOLEAN);
	return lua_toboolean(ctx->lua, idx + 1) ? 1 : 0;
}

static String8 S_CtxGetArgStr(S_Context *ctx, u32 idx)
{
	u64 len = 0;
	const char *s = luaL_checklstring(ctx->lua, idx + 1, &len);
	return String8Init(s, len);
}

static u32 S_CtxGetArgTaggedU32(S_Context *ctx, u32 idx, u32 expected_tag)
{
	lua_Integer packed = luaL_checkinteger(ctx->lua, idx + 1);

	u32 value = 0;
	u32 tag = 0;

	S_UnpackTaggedU32(packed, &value, &tag);

	if (tag != expected_tag)
	{
		luaL_error(ctx->lua,
				   "Argument %u: Expected tag 0x%X, got 0x%X.",
				   idx, expected_tag, tag);
	}

	return value;
}

static f32 S_CtxGetArgF32Opt(S_Context *ctx, u32 idx, f32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	return luaL_checknumber(ctx->lua, idx + 1);
}

static i32 S_CtxGetArgI32Opt(S_Context *ctx, u32 idx, i32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	return luaL_checkinteger(ctx->lua, idx + 1);
}

static b32 S_CtxGetArgB32Opt(S_Context *ctx, u32 idx, b32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	luaL_checktype(ctx->lua, idx + 1, LUA_TBOOLEAN);
	return lua_toboolean(ctx->lua, idx + 1) ? true : false;
}

static String8 S_CtxGetArgStrOpt(S_Context *ctx, u32 idx, String8 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	u64 len = 0;
	const char *s = luaL_checklstring(ctx->lua, idx + 1, &len);

	return String8Init(s, len);
}

static void *S_CtxUpvaluePtr(S_Context *ctx, u32 idx)
{
	i32 up_index = lua_upvalueindex(idx + S_USER_UPVAL_BASE);
	return lua_touserdata(ctx->lua, up_index);
}

static void S_CtxReturnNil(S_Context *ctx)
{
	lua_pushnil(ctx->lua);
	ctx->nretval++;
}

static void S_CtxReturnF32(S_Context *ctx, f32 v)
{
	lua_pushnumber(ctx->lua, v);
	ctx->nretval++;
}

static void S_CtxReturnI32(S_Context *ctx, i32 v)
{
	lua_pushinteger(ctx->lua, v);
	ctx->nretval++;
}

static void S_CtxReturnB32(S_Context *ctx, b32 v)
{
	lua_pushboolean(ctx->lua, v ? 1 : 0);
	ctx->nretval++;
}

static void S_CtxReturnString8(S_Context *ctx, String8 v)
{
	lua_pushlstring(ctx->lua, (const char *)v.str, v.len);
	ctx->nretval++;
}

static void S_CtxReturnTaggedU32(S_Context *ctx, u32 v, u32 type_tag)
{
	lua_Integer packed = (lua_Integer)(((u64)type_tag << 32) | (u64)v);
	lua_pushinteger(ctx->lua, packed);
	ctx->nretval++;
}

static void S_CtxYield(S_Context *ctx)
{
	S_Coroutine *co = ctx->system->curr;

	DebugLogAssert(ctx->system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(ctx->system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = S_YieldKind_Tick;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = 0;

	lua_yield(ctx->lua, 0);
}

static void S_CtxYieldTime(S_Context *ctx, f32 time_s)
{
	S_Coroutine *co = ctx->system->curr;

	DebugLogAssert(ctx->system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(ctx->system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = S_YieldKind_TimeSeconds;
	co->wait_remaining_s = time_s;
	co->wait_signal_hash = 0;

	lua_yield(ctx->lua, 0);
}

static void S_CtxYieldSignal(S_Context *ctx, String8 name)
{
	S_Coroutine *co = ctx->system->curr;

	DebugLogAssert(ctx->system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(ctx->system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = S_YieldKind_Signal;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = HashStr8(name);

	lua_yield(ctx->lua, 0);
}

static i32 S_CtxContinuationTrampoline(lua_State *lua, i32 status, lua_KContext kctx)
{
	S_System *sys = S_GetSystemFromL(lua);
	S_Coroutine *co  = sys->curr;

	S_ContinueFn *cont = co->continue_fn;
	void *user_data = co->continue_user_data;

	co->continue_fn = NULL;
	co->continue_user_data = NULL;

	S_Context ctx = {0};
	ctx.system = sys;
	ctx.lua = lua;
	ctx.nretval = 0;

	if (cont)
		cont(&ctx, user_data);

	return ctx.nretval;
}

static void S_CtxYieldSignalCont(S_Context *ctx, String8 name, S_ContinueFn *cont, void *user_data)
{
	S_Coroutine *co = ctx->system->curr;

	DebugLogAssert(ctx->system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(ctx->system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = S_YieldKind_Signal;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = HashStr8(name);
	co->continue_fn = cont;
	co->continue_user_data = user_data;

	lua_yieldk(ctx->lua, 0, 0, S_CtxContinuationTrampoline);
}

static void S_CtxThrow(S_Context *ctx, const char *fmt, ...)
{
	char buf[512] = {0};

	va_list param;
	va_start(param, fmt);
	vsnprintf(buf, sizeof(buf), fmt, param);
	va_end(param);

	luaL_error(ctx->lua, "%s", buf);
}

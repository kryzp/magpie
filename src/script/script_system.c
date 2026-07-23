
static S_System *s_system = NULL;

// lua indices start at 1
// first index reserved for function pointer
// start from 2
#define S_USER_UPVAL_BASE 2

typedef enum S_YieldKind
{
	S_YieldKind_None,
	S_YieldKind_Tick,         // resume on the next tick
	S_YieldKind_TimeSeconds,  // resume after wait_remaining_s reaches zero
	S_YieldKind_Signal,       // resume when wait_signal_hash fires
	S_YieldKind_Children,     // resume when active_children reaches zero
	S_YieldKind_COUNT
}
S_YieldKind;

typedef struct S_Context S_Context;
struct S_Context
{
	lua_State *lua;
	i32 nretval;
};

typedef struct S_Coroutine S_Coroutine;
struct S_Coroutine
{
	b32 in_use;
	u32 generation;

	lua_State *thread;
	i32 thread_ref; // NOT AN OS THREAD. luaL_ref into LUA_REGISTRYINDEX

	b32 has_been_resumed;
	u32 initial_arg_count;

	S_YieldKind yield_kind;
	f32 wait_remaining_s;
	u64 wait_signal_hash;

	i32 parent; // -1 = none
	u32 active_children;

	S_ContinueFn *continue_fn;
	void *continue_user_data;

	S_FinishFn *finish_fn;
	void *finish_user_data;
};

typedef struct S_System S_System;
struct S_System
{
	Arena *arena;
	LOG_Channel log_channel;

	lua_State *lua;

	S_Coroutine coroutines[S_MAX_COROUTINES];
	u32 first_free_hint;
	S_Coroutine *curr; // set for the duration of a resume

	u64 pending_signals[S_MAX_PENDING_SIGNALS];
	u32 pending_signal_count;
};

internal S_Coroutine *S_AllocCoroutine(S_Handle *out_handle)
{
	// 0 = null
	for (u32 i = 1; i < S_MAX_COROUTINES; i++)
	{
		u32 slot = (s_system->first_free_hint + i) % S_MAX_COROUTINES;

		if (slot == 0)
			continue;

		S_Coroutine *co = &s_system->coroutines[slot];

		if (!co->in_use)
		{
			co->in_use             = true;
			co->has_been_resumed   = false;
			co->initial_arg_count  = 0;
			
			co->yield_kind         = S_YieldKind_Tick;
			co->wait_remaining_s   = 0.f;
			co->wait_signal_hash   = 0;

			co->parent             = -1;
			co->active_children    = 0;

			co->continue_fn        = NULL;
			co->continue_user_data = NULL;

			co->finish_fn          = NULL;
			co->finish_user_data   = NULL;

			co->thread             = NULL;
			co->thread_ref         = LUA_NOREF;

			s_system->first_free_hint = slot;

			if (out_handle)
			{
				out_handle->index = slot;
				out_handle->generation = co->generation;
			}

			return co;
		}
	}

	DebugLogE(s_system->log_channel, "Out of coroutine slots (max %u).", ArraySize(s_system->coroutines));

	if (out_handle)
		*out_handle = S_HandleNull();

	return NULL;
}

internal void S_FreeCoroutine(S_Coroutine *co)
{
	if (co->finish_fn)
		co->finish_fn(co, co->finish_user_data);

	if (co->parent >= 0 && co->parent < (i32)S_MAX_COROUTINES)
	{
		S_Coroutine *parent = &s_system->coroutines[co->parent];

		if (parent->in_use && parent->active_children > 0)
			parent->active_children--;
	}

	if (co->thread_ref != LUA_NOREF)
	{
		luaL_unref(s_system->lua, LUA_REGISTRYINDEX, co->thread_ref);
		co->thread_ref = LUA_NOREF;
	}

	co->thread = NULL;

	co->in_use = false;
	co->generation++;
	co->yield_kind = S_YieldKind_None;
}

internal S_Coroutine *S_ResolveHandle(S_Handle handle)
{
	if (S_HandleIsNull(handle))
		return NULL;

	if (handle.index == 0 || handle.index >= S_MAX_COROUTINES)
		return NULL;

	S_Coroutine *co = &s_system->coroutines[handle.index];

	if (!co->in_use)
		return NULL;

	if (co->generation != handle.generation)
		return NULL;

	return co;
}

internal i32 S_BindingTrampoline(lua_State *lua)
{
	S_BindingFn *fn = (S_BindingFn *)lua_touserdata(lua, lua_upvalueindex(1));

	S_Context ctx = {0};
	ctx.lua = lua;
	ctx.nretval = 0;

	if (fn)
		fn(&ctx);

	return ctx.nretval;
}

internal void S_PushBindingClosure(S_BindingFn *fn, void *const *upvalues, u32 n)
{
	// first upval is reserved for the function pointer
	lua_pushlightuserdata(s_system->lua, (void *)fn);

	for (u32 i = 0; i < n; i++)
		lua_pushlightuserdata(s_system->lua, upvalues ? upvalues[i] : NULL);

	lua_pushcclosure(s_system->lua, S_BindingTrampoline, S_USER_UPVAL_BASE - 1 + n);
}

internal void S_PushArg(lua_State *target, const S_Argument *a)
{
	switch (a->type)
	{
		case S_ArgType_Nil:
			lua_pushnil(target);
			break;
			
		case S_ArgType_F32:
			lua_pushnumber(target, a->float32);
			break;
			
		case S_ArgType_I32:
			lua_pushinteger(target, a->int32);
			break;
			
		case S_ArgType_B32:
			lua_pushboolean(target, a->bool32 ? 1 : 0);
			break;
			
		case S_ArgType_String8:
			lua_pushlstring(target, (const char *)a->string8.str, a->string8.len);
			break;

		case S_ArgType_TaggedU32:
			{
				lua_Integer packed = (lua_Integer)(((u64)a->tagged32.tag << 32) | (u64)a->tagged32.unsigned32);
				lua_pushinteger(target, packed);
			}
			break;

		default:
			lua_pushnil(target);
			break;
	}
}

internal S_BINDING_DEF(S_BND_Dispatch)
{
	u32 n = S_CtxGetArgCount(ctx);

	if (n == 0)
	{
		S_CtxThrow(ctx, "dispatch() has no parameters.");
		return;
	}
	
	S_Coroutine *parent = s_system->curr;

	if (!parent)
	{
		S_CtxThrow(ctx, "dispatch() can only be called from inside a coroutine.");
		return;
	}
	
	S_Handle child_handle;
	S_Coroutine *child = S_AllocCoroutine(&child_handle);
	
	if (!child)
	{
		DebugLogE(s_system->log_channel, "Coroutine pool exhausted.");
		return;
	}

	child->thread = lua_newthread(s_system->lua);
	child->thread_ref = luaL_ref(s_system->lua, LUA_REGISTRYINDEX);

	// push function and arguments onto new thread stack
	for (u32 i = 1; i <= n; i++)
		lua_pushvalue(ctx->lua, i);

	lua_xmove(ctx->lua, child->thread, n);

	child->has_been_resumed = false;
	child->initial_arg_count = n;
	child->yield_kind = S_YieldKind_Tick;
	child->parent = -1;
}

internal S_BINDING_DEF(S_BND_Parallelize)
{
	u32 n = S_CtxGetArgCount(ctx);

	if (n == 0)
	{
		S_CtxThrow(ctx, "parallelize() has no parameters.");
		return;
	}
	
	S_Coroutine *parent = s_system->curr;

	if (!parent)
	{
		S_CtxThrow(ctx, "parallelize() can only be called from inside a coroutine.");
		return;
	}

	parent->active_children = n;

	const i32 parent_idx = (i32)(parent - s_system->coroutines);

	for (u32 i = 0; i < n; i++)
	{
		S_Handle child_handle;
		S_Coroutine *child = S_AllocCoroutine(&child_handle);
		
		if (!child)
		{
			parent->active_children--;
			DebugLogE(s_system->log_channel, "Coroutine pool exhausted.");
			continue;
		}

		child->thread = lua_newthread(s_system->lua);
		child->thread_ref = luaL_ref(s_system->lua, LUA_REGISTRYINDEX);

		lua_pushvalue(ctx->lua, i + 1); 
		lua_xmove(ctx->lua, child->thread, 1);

		child->has_been_resumed = false;
		child->initial_arg_count = 0;
		child->yield_kind = S_YieldKind_Tick;
		child->parent = parent_idx;
	}

	parent->yield_kind = S_YieldKind_Children;
	lua_yield(ctx->lua, 0);
}

internal void *S_InternalLuaAllocator(void *ud, void *ptr, usize old_size, usize new_size)
{
	if (new_size == 0)
	{
		osapi->HeapFree(ptr);
		return NULL;
	}

	if (!ptr)
	{
		return osapi->HeapAlloc(new_size);
	}
	else
	{
		return osapi->HeapRealloc(ptr, new_size);
	}
}

internal S_System *S_AllocAndSelect(Arena *arena, LOG_Channel log_channel)
{
	S_System *system = ArenaPushArray(arena, S_System, 1);

	system->arena = arena;
	system->log_channel = log_channel;

	system->lua = lua_newstate(S_InternalLuaAllocator, NULL, 0);

	DebugLogAssert(system->log_channel, system->lua, "Lua failed to initialize (still NULL).");

	luaL_openlibs(system->lua);

	system->first_free_hint = 0;

	for (u32 i = 0; i < S_MAX_COROUTINES; i++)
	{
		system->coroutines[i].in_use = false;
		system->coroutines[i].generation = 1; // 0 = null
		system->coroutines[i].thread_ref = LUA_NOREF;
		system->coroutines[i].parent = -1;
	}

	system->pending_signal_count = 0;

	S_SelectContext(system);

	S_BindGlobal(String8Lit("dispatch"), S_BND_Dispatch);
	S_BindGlobal(String8Lit("parallelize"), S_BND_Parallelize);

	DebugLogI(system->log_channel, "Initialized using %s.", LUA_VERSION);

	return system;
}

internal void S_Destroy(void)
{
	if (!s_system->lua)
		return;

	for (u32 i = 0; i < S_MAX_COROUTINES; i++)
	{
		if (s_system->coroutines[i].in_use)
			S_FreeCoroutine(&s_system->coroutines[i]);
	}

	lua_close(s_system->lua);

	DebugLogI(s_system->log_channel, "Destroyed.");

	s_system = NULL;
}

internal void S_SelectContext(S_System *system)
{
	s_system = system;
}

internal S_Ref S_Compile(IO_ByteSpan source, String8 chunk_name)
{
	S_Ref result = S_RefNull();

	i32 status = luaL_loadbufferx(s_system->lua, (const char *)source.bytes, source.size, (const char *)chunk_name.str, "t");

	if (status != LUA_OK)
	{
		const char *err = lua_tostring(s_system->lua, -1);

		DebugLogE(s_system->log_channel,
				  "Compile failed (%.*s): %s",
				  String8VArg(chunk_name), err ? err : "(no message)");

		lua_pop(s_system->lua, 1);
	}
	else
	{
		result.value = luaL_ref(s_system->lua, LUA_REGISTRYINDEX);
	}

	return result;
}

internal void S_Release(S_Ref ref)
{
	if (S_RefIsNull(ref))
		return;

	luaL_unref(s_system->lua, LUA_REGISTRYINDEX, ref.value);
}

internal S_Ref S_ExecuteModule(S_Ref chunk_ref)
{
	S_Ref result = S_RefNull();

	if (S_RefIsNull(chunk_ref))
		goto end;

	lua_rawgeti(s_system->lua, LUA_REGISTRYINDEX, chunk_ref.value);
	
	if (lua_pcall(s_system->lua, 0, 1, 0) != LUA_OK)
	{
		DebugLogE(s_system->log_channel, "Module init error: %s", lua_tostring(s_system->lua, -1));

		lua_pop(s_system->lua, 1);
		goto end;
	}

	if (!lua_istable(s_system->lua, -1))
	{
		DebugLogE(s_system->log_channel, "Script did not return a table.");

		lua_pop(s_system->lua, 1);
		goto end;
	}

	result.value = luaL_ref(s_system->lua, LUA_REGISTRYINDEX);

end:
	return result;
}

internal S_Ref S_NewInstance(S_Ref module_ref)
{
	S_Ref result = S_RefNull();

	if (S_RefIsNull(module_ref))
		goto end;

	lua_newtable(s_system->lua); // instance
	lua_newtable(s_system->lua); // metatable
	lua_rawgeti(s_system->lua, LUA_REGISTRYINDEX, module_ref.value);
	lua_setfield(s_system->lua, -2, "__index");
	lua_setmetatable(s_system->lua, -2);
	
	result.value = luaL_ref(s_system->lua, LUA_REGISTRYINDEX);

end:
	return result;
}

internal S_Handle S_PlayCallableOnThread(lua_State *thread, i32 thread_ref, u32 arg_count)
{
	S_Handle handle = {0};
	S_Coroutine *co = S_AllocCoroutine(&handle);

	if (!co)
	{
		luaL_unref(s_system->lua, LUA_REGISTRYINDEX, thread_ref);
		return S_HandleNull();
	}

	co->thread = thread;
	co->thread_ref = thread_ref;
	co->initial_arg_count = arg_count;
	co->yield_kind = S_YieldKind_Tick;
	co->has_been_resumed = false;

	return handle;
}

internal S_Handle S_CallMethod(S_Ref instance_ref, String8 method_name)
{
	return S_CallMethodEx(instance_ref, method_name, NULL, 0);
}

internal S_Handle S_CallMethodEx(S_Ref instance_ref, String8 method_name, const S_Argument *args, u32 arg_count)
{
	if (S_RefIsNull(instance_ref))
		return S_HandleNull();

	lua_State *thread = lua_newthread(s_system->lua);
	i32 thread_ref = luaL_ref(s_system->lua, LUA_REGISTRYINDEX);

	lua_rawgeti(thread, LUA_REGISTRYINDEX, instance_ref.value); // self
	lua_getfield(thread, -1, (const char *)method_name.str);

	if (lua_type(thread, -1) != LUA_TFUNCTION)
	{
		DebugLogW(s_system->log_channel, "Method '%.*s' not found or not a function.", String8VArg(method_name));

		lua_settop(thread, 0);
		luaL_unref(s_system->lua, LUA_REGISTRYINDEX, thread_ref);
		return S_HandleNull();
	}

	// ok basically lua automaticlaly does this:
	// foo:Bar(...) becomes Bar(foo, ...)
	// i.e: object:Method() -> Method(object, ...)
	// so we gotta swap them before we call the method
	lua_insert(thread, -2);

	for (u32 i = 0; i < arg_count; i++)
		S_PushArg(thread, &args[i]);

	return S_PlayCallableOnThread(thread, thread_ref, arg_count + 1); // +1 bcuz of self
}

internal void S_Stop(S_Handle handle)
{
	S_Coroutine *co = S_ResolveHandle(handle);

	if (!co)
		return;

	S_FreeCoroutine(co);
}

internal b32 S_IsRunning(S_Handle handle)
{
	return S_ResolveHandle(handle) != NULL;
}

internal void S_FireSignal(String8 name)
{
	if (s_system->pending_signal_count >= S_MAX_PENDING_SIGNALS)
	{
		DebugLogE(s_system->log_channel,
				  "Signal queue full, dropping signal \"%.*s\".",
				  String8VArg(name));

		return;
	}

	u64 hash = HashStr8(name);
	s_system->pending_signals[s_system->pending_signal_count++] = hash;
}

internal b32 S_SignalIsPending(u64 hash)
{
	for (u32 i = 0; i < s_system->pending_signal_count; i++)
	{
		if (s_system->pending_signals[i] == hash)
			return true;
	}

	return false;
}

internal b32 S_CoroutineIsReady(const S_Coroutine *co)
{
	if (!co->in_use)
		return false;

	switch (co->yield_kind)
	{
		case S_YieldKind_Tick:
			return true;

		case S_YieldKind_TimeSeconds:
			return co->wait_remaining_s <= 0.f;

		case S_YieldKind_Signal:
			return S_SignalIsPending(co->wait_signal_hash);

		case S_YieldKind_Children:
			return co->active_children == 0;

		default:
			return false;
	}
}

internal void S_ResumeOne(S_Coroutine *co)
{
	s_system->curr = co;

	i32 nargs = 0;
	
	if (!co->has_been_resumed)
	{
		nargs = co->initial_arg_count;
		co->has_been_resumed = true;
	}

	i32 nres = 0;
	i32 status = lua_resume(co->thread, s_system->lua, nargs, &nres);

	s_system->curr = NULL;

	if (status == LUA_OK)
	{
		if (nres > 0)
			lua_pop(co->thread, nres);
		
		S_FreeCoroutine(co);
	}
	else if (status == LUA_YIELD)
	{
		if (nres > 0)
			lua_pop(co->thread, nres);
	}
	else
	{
		const char *raw_msg = lua_tostring(co->thread, -1);

		if (!raw_msg) 
			raw_msg = "(no message)";

		luaL_traceback(s_system->lua, co->thread, raw_msg, 0);
		const char *full_trace = lua_tostring(s_system->lua, -1);

		DebugLogE(s_system->log_channel, "%s", full_trace);

		lua_pop(s_system->lua, 1); // pop the traceback string from main state
		lua_pop(co->thread, 1);  // pop the original error object from the coroutine state

		S_FreeCoroutine(co);
	}
}

internal void S_Tick(f32 dt)
{
	if (!system || !s_system->lua)
		return;

	for (u32 i = 1; i < S_MAX_COROUTINES; i++)
	{
		S_Coroutine *co = &s_system->coroutines[i];

		if (!co->in_use)
			continue;

		if (co->yield_kind == S_YieldKind_TimeSeconds)
		{
			co->wait_remaining_s -= dt;

			if (co->wait_remaining_s < 0.f)
				co->wait_remaining_s = 0.f;
		}
	}

	for (u32 i = 1; i < S_MAX_COROUTINES; i++)
	{
		S_Coroutine *co = &s_system->coroutines[i];

		if (!S_CoroutineIsReady(co))
			continue;

		S_ResumeOne(co);
	}

	s_system->pending_signal_count = 0;
}

internal void S_SetOnFinish(S_Handle handle, S_FinishFn *fn, void *user_data)
{
	S_Coroutine *co = S_ResolveHandle(handle);

	if (!co)
		return;

	co->finish_fn = fn;
	co->finish_user_data = user_data;
}

internal void S_BindGlobal(String8 name, S_BindingFn *fn)
{
	S_BindGlobalEx(name, fn, NULL, 0);
}

internal void S_BindToTable(String8 table, String8 name, S_BindingFn *fn)
{
	S_BindToTableEx(table, name, fn, NULL, 0);
}

internal void S_BindGlobalEx(String8 name, S_BindingFn *fn, void *const *upvalues, u32 n)
{
	S_PushBindingClosure(fn, upvalues, n);
	lua_setglobal(s_system->lua, (const char *)name.str);
}

internal void S_BindToTableEx(String8 table, String8 name, S_BindingFn *fn, void *const *upvalues, u32 n)
{
	if (lua_getglobal(s_system->lua, (const char *)table.str) != LUA_TTABLE)
	{
		lua_pop(s_system->lua, 1);
		lua_newtable(s_system->lua);
		lua_pushvalue(s_system->lua, -1);
		lua_setglobal(s_system->lua, (const char *)table.str);
	}

	S_PushBindingClosure(fn, upvalues, n);
	lua_setfield(s_system->lua, -2, (const char *)name.str);
	lua_pop(s_system->lua, 1);
}

internal u32 S_CtxGetArgCount(S_Context *ctx)
{
	return lua_gettop(ctx->lua);
}

internal f32 S_CtxGetArgF32(S_Context *ctx, u32 idx)
{
	return luaL_checknumber(ctx->lua, idx + 1);
}

internal i32 S_CtxGetArgI32(S_Context *ctx, u32 idx)
{
	return luaL_checkinteger(ctx->lua, idx + 1);
}

internal b32 S_CtxGetArgBool(S_Context *ctx, u32 idx)
{
	luaL_checktype(ctx->lua, idx + 1, LUA_TBOOLEAN);
	return lua_toboolean(ctx->lua, idx + 1) ? 1 : 0;
}

internal String8 S_CtxGetArgStr(S_Context *ctx, u32 idx)
{
	u64 len = 0;
	const char *s = luaL_checklstring(ctx->lua, idx + 1, &len);
	return String8Init(s, len);
}

internal u32 S_CtxGetArgTaggedU32(S_Context *ctx, u32 idx, u32 expected_tag)
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

internal f32 S_CtxGetArgF32Opt(S_Context *ctx, u32 idx, f32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	return luaL_checknumber(ctx->lua, idx + 1);
}

internal i32 S_CtxGetArgI32Opt(S_Context *ctx, u32 idx, i32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	return luaL_checkinteger(ctx->lua, idx + 1);
}

internal b32 S_CtxGetArgB32Opt(S_Context *ctx, u32 idx, b32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	luaL_checktype(ctx->lua, idx + 1, LUA_TBOOLEAN);
	return lua_toboolean(ctx->lua, idx + 1) ? true : false;
}

internal String8 S_CtxGetArgStrOpt(S_Context *ctx, u32 idx, String8 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	u64 len = 0;
	const char *s = luaL_checklstring(ctx->lua, idx + 1, &len);

	return String8Init(s, len);
}

internal void *S_CtxUpvaluePtr(S_Context *ctx, u32 idx)
{
	i32 up_index = lua_upvalueindex(idx + S_USER_UPVAL_BASE);
	return lua_touserdata(ctx->lua, up_index);
}

internal void S_CtxReturnNil(S_Context *ctx)
{
	lua_pushnil(ctx->lua);
	ctx->nretval++;
}

internal void S_CtxReturnF32(S_Context *ctx, f32 v)
{
	lua_pushnumber(ctx->lua, v);
	ctx->nretval++;
}

internal void S_CtxReturnI32(S_Context *ctx, i32 v)
{
	lua_pushinteger(ctx->lua, v);
	ctx->nretval++;
}

internal void S_CtxReturnB32(S_Context *ctx, b32 v)
{
	lua_pushboolean(ctx->lua, v ? 1 : 0);
	ctx->nretval++;
}

internal void S_CtxReturnString8(S_Context *ctx, String8 v)
{
	lua_pushlstring(ctx->lua, (const char *)v.str, v.len);
	ctx->nretval++;
}

internal void S_CtxReturnTaggedU32(S_Context *ctx, u32 v, u32 type_tag)
{
	lua_Integer packed = (lua_Integer)(((u64)type_tag << 32) | (u64)v);
	lua_pushinteger(ctx->lua, packed);
	ctx->nretval++;
}

internal void S_CtxYield(S_Context *ctx)
{
	S_Coroutine *co = s_system->curr;

	DebugLogAssert(s_system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(s_system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = S_YieldKind_Tick;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = 0;

	lua_yield(ctx->lua, 0);
}

internal void S_CtxYieldTime(S_Context *ctx, f32 time_s)
{
	S_Coroutine *co = s_system->curr;

	DebugLogAssert(s_system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(s_system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = S_YieldKind_TimeSeconds;
	co->wait_remaining_s = time_s;
	co->wait_signal_hash = 0;

	lua_yield(ctx->lua, 0);
}

internal void S_CtxYieldSignal(S_Context *ctx, String8 name)
{
	S_Coroutine *co = s_system->curr;

	DebugLogAssert(s_system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(s_system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = S_YieldKind_Signal;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = HashStr8(name);

	lua_yield(ctx->lua, 0);
}

internal i32 S_CtxContinuationTrampoline(lua_State *lua, i32 status, lua_KContext kctx)
{
	S_Coroutine *co = s_system->curr;

	S_ContinueFn *cont = co->continue_fn;
	void *user_data = co->continue_user_data;

	co->continue_fn = NULL;
	co->continue_user_data = NULL;

	S_Context ctx = {0};
	ctx.lua = lua;
	ctx.nretval = 0;

	if (cont)
		cont(&ctx, user_data);

	return ctx.nretval;
}

internal void S_CtxYieldSignalCont(S_Context *ctx, String8 name, S_ContinueFn *cont, void *user_data)
{
	S_Coroutine *co = s_system->curr;

	DebugLogAssert(s_system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(s_system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = S_YieldKind_Signal;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = HashStr8(name);
	co->continue_fn = cont;
	co->continue_user_data = user_data;

	lua_yieldk(ctx->lua, 0, 0, S_CtxContinuationTrampoline);
}

internal void S_CtxThrow(S_Context *ctx, const char *fmt, ...)
{
	char buf[512] = {0};

	va_list param;
	va_start(param, fmt);
	vsnprintf(buf, sizeof(buf), fmt, param);
	va_end(param);

	luaL_error(ctx->lua, "%s", buf);
}

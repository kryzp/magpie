
// lua indices start at 1
// first index reserved for function pointer
// start from 2
#define SCR_USER_UPVAL_BASE 2

typedef enum SCR_YieldKind
{
	SCR_YieldKind_None,
	SCR_YieldKind_Tick,         // resume on the next tick
	SCR_YieldKind_TimeSeconds,  // resume after wait_remaining_s reaches zero
	SCR_YieldKind_Signal,       // resume when wait_signal_hash fires
	SCR_YieldKind_Children,     // resume when active_children reaches zero
	SCR_YieldKind_COUNT
}
SCR_YieldKind;

typedef struct SCR_Context SCR_Context;
struct SCR_Context
{
	SCR_System *system;
	lua_State *lua;
	i32 nretval;
};

typedef struct SCR_Coroutine SCR_Coroutine;
struct SCR_Coroutine
{
	b32 in_use;
	u32 generation;

	lua_State *thread;
	i32 thread_ref; // NOT AN OS THREAD. luaL_ref into LUA_REGISTRYINDEX

	b32 has_been_resumed;
	u32 initial_arg_count;

	SCR_YieldKind yield_kind;
	f32 wait_remaining_s;
	u64 wait_signal_hash;

	i32 parent; // -1 = none
	u32 active_children;

	SCR_ContinueFn *continue_fn;
	void *continue_user_data;

	SCR_FinishFn *finish_fn;
	void *finish_user_data;
};

typedef struct SCR_System SCR_System;
struct SCR_System
{
	Arena *arena;
	LOG_Channel log_channel;

	lua_State *lua;

	SCR_Coroutine coroutines[SCR_MAX_COROUTINES];
	u32 first_free_hint;
	SCR_Coroutine *curr; // set for the duration of a resume

	u64 pending_signals[SCR_MAX_PENDING_SIGNALS];
	u32 pending_signal_count;
};

// kinda hacky but every child inherits a copy from the main
// thread then its created so that the bindings can grab the
// system pointer in O(1) regardless of the current coroutine.

internal SCR_System *
SCR_GetSystemFromL(lua_State *lua)
{
	return *((SCR_System **)lua_getextraspace(lua));
}

internal void
SCR_SetSystemInL(lua_State *lua, SCR_System *system)
{
	*((SCR_System **)lua_getextraspace(lua)) = system;
}

internal SCR_Coroutine *
SCR_AllocCoroutine(SCR_System *system, SCR_Handle *out_handle)
{
	// 0 = null
	for (u32 i = 1; i < SCR_MAX_COROUTINES; i++)
	{
		u32 slot = (system->first_free_hint + i) % SCR_MAX_COROUTINES;

		if (slot == 0)
			continue;

		SCR_Coroutine *co = &system->coroutines[slot];

		if (!co->in_use)
		{
			co->in_use             = true;
			co->has_been_resumed   = false;
			co->initial_arg_count  = 0;
			
			co->yield_kind         = SCR_YieldKind_Tick;
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

			system->first_free_hint = slot;

			if (out_handle)
			{
				out_handle->index = slot;
				out_handle->generation = co->generation;
			}

			return co;
		}
	}

	DebugLogE(system->log_channel, "Out of coroutine slots (max %u).", ArraySize(system->coroutines));

	if (out_handle)
		*out_handle = SCR_HandleNull();

	return NULL;
}

internal void
SCR_FreeCoroutine(SCR_System *system, SCR_Coroutine *co)
{
	if (co->finish_fn)
		co->finish_fn(system, co, co->finish_user_data);

	if (co->parent >= 0 && co->parent < (i32)SCR_MAX_COROUTINES)
	{
		SCR_Coroutine *parent = &system->coroutines[co->parent];

		if (parent->in_use && parent->active_children > 0)
			parent->active_children--;
	}

	if (co->thread_ref != LUA_NOREF)
	{
		luaL_unref(system->lua, LUA_REGISTRYINDEX, co->thread_ref);
		co->thread_ref = LUA_NOREF;
	}

	co->thread = NULL;

	co->in_use = false;
	co->generation++;
	co->yield_kind = SCR_YieldKind_None;
}

internal SCR_Coroutine *
SCR_ResolveHandle(SCR_System *system, SCR_Handle handle)
{
	if (SCR_HandleIsNull(handle))
		return NULL;

	if (handle.index == 0 || handle.index >= SCR_MAX_COROUTINES)
		return NULL;

	SCR_Coroutine *co = &system->coroutines[handle.index];

	if (!co->in_use)
		return NULL;

	if (co->generation != handle.generation)
		return NULL;

	return co;
}

internal i32
SCR_BindingTrampoline(lua_State *lua)
{
	SCR_System *sys = SCR_GetSystemFromL(lua);
	SCR_BindingFn *fn  = (SCR_BindingFn *)lua_touserdata(lua, lua_upvalueindex(1));

	SCR_Context ctx = {0};
	ctx.system = sys;
	ctx.lua = lua;
	ctx.nretval = 0;

	if (fn)
		fn(&ctx);

	return ctx.nretval;
}

internal void
SCR_PushBindingClosure(SCR_System *system, SCR_BindingFn *fn,
					   void * const *upvalues, u32 n)
{
	// first upval is reserved for the function pointer
	lua_pushlightuserdata(system->lua, (void *)fn);

	for (u32 i = 0; i < n; i++)
		lua_pushlightuserdata(system->lua, upvalues ? upvalues[i] : NULL);

	lua_pushcclosure(system->lua, SCR_BindingTrampoline, SCR_USER_UPVAL_BASE - 1 + n);
}

internal void
SCR_PushArg(lua_State *target, const SCR_Argument *a)
{
	switch (a->type)
	{
		case SCR_ArgType_Nil:
			lua_pushnil(target);
			break;
			
		case SCR_ArgType_F32:
			lua_pushnumber(target, a->float32);
			break;
			
		case SCR_ArgType_I32:
			lua_pushinteger(target, a->int32);
			break;
			
		case SCR_ArgType_B32:
			lua_pushboolean(target, a->bool32 ? 1 : 0);
			break;
			
		case SCR_ArgType_String8:
			lua_pushlstring(target, (const char *)a->string8.str, a->string8.len);
			break;

		case SCR_ArgType_TaggedU32:
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

SCR_BINDING_DEF(SCR_BND_Dispatch)
{
	SCR_System *sys = ctx->system;
	u32 n = SCR_GetArgCount(ctx);

	if (n == 0)
	{
		SCR_Throw(ctx, "dispatch() has no parameters.");
		return;
	}
	
	SCR_Coroutine *parent = sys->curr;

	if (!parent)
	{
		SCR_Throw(ctx, "dispatch() can only be called from inside a coroutine.");
		return;
	}
	
	SCR_Handle child_handle;
	SCR_Coroutine *child = SCR_AllocCoroutine(sys, &child_handle);
	
	if (!child)
	{
		DebugLogE(sys->log_channel, "Coroutine pool exhausted.");
		return;
	}

	child->thread = lua_newthread(sys->lua);
	child->thread_ref = luaL_ref(sys->lua, LUA_REGISTRYINDEX);

	lua_pushvalue(ctx->lua, 1);
	lua_xmove(ctx->lua, child->thread, 1);

	child->has_been_resumed = false;
	child->initial_arg_count = 0;
	child->yield_kind = SCR_YieldKind_Tick;
	child->parent = -1;
}

SCR_BINDING_DEF(SCR_BND_Parallelize)
{
	SCR_System *sys = ctx->system;
	u32 n = SCR_GetArgCount(ctx);

	if (n == 0)
	{
		SCR_Throw(ctx, "parallelize() has no parameters.");
		return;
	}
	
	SCR_Coroutine *parent = sys->curr;

	if (!parent)
	{
		SCR_Throw(ctx, "parallelize() can only be called from inside a coroutine.");
		return;
	}

	parent->active_children = n;

	for (u32 i = 0; i < n; i++)
	{
		SCR_Handle child_handle;
		SCR_Coroutine *child = SCR_AllocCoroutine(sys, &child_handle);
		
		if (!child)
		{
			parent->active_children--;
			DebugLogE(sys->log_channel, "Coroutine pool exhausted.");
			continue;
		}

		child->thread = lua_newthread(sys->lua);
		child->thread_ref = luaL_ref(sys->lua, LUA_REGISTRYINDEX);

		lua_pushvalue(ctx->lua, i + 1); 
		lua_xmove(ctx->lua, child->thread, 1);

		child->has_been_resumed = false;
		child->initial_arg_count = 0;
		child->yield_kind = SCR_YieldKind_Tick;
		child->parent = (i32)(parent - &sys->coroutines[0]);
	}

	parent->yield_kind = SCR_YieldKind_Children;
	lua_yield(ctx->lua, 0);
}

internal void *
SCR_InternalLuaAllocator(void *ud, void *ptr, usize old_size, usize new_size)
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

internal SCR_System *
SCR_Init(Arena *arena, LOG_Channel log_channel)
{
	SCR_System *system = ArenaPushArray(arena, SCR_System, 1);

	system->arena = arena;
	system->log_channel = log_channel;

	system->lua = lua_newstate(SCR_InternalLuaAllocator, NULL, 0);

	DebugLogAssert(system->log_channel, system->lua, "Lua failed to initialize (still NULL).");

	SCR_SetSystemInL(system->lua, system);

	luaL_openlibs(system->lua);

	system->first_free_hint = 0;

	for (u32 i = 0; i < SCR_MAX_COROUTINES; i++)
	{
		system->coroutines[i].in_use = false;
		system->coroutines[i].generation = 1; // 0 = null
		system->coroutines[i].thread_ref = LUA_NOREF;
		system->coroutines[i].parent = -1;
	}

	system->pending_signal_count = 0;

	SCR_BindGlobal(system, String8Lit("dispatch"),     SCR_BND_Dispatch);
	SCR_BindGlobal(system, String8Lit("parallelize"),  SCR_BND_Parallelize);

	DebugLogI(system->log_channel, "%s initialized.", LUA_VERSION);

	return system;
}

internal void
SCR_Destroy(SCR_System *system)
{
	if (!system || !system->lua)
		return;

	for (u32 i = 0; i < SCR_MAX_COROUTINES; i++)
	{
		if (system->coroutines[i].in_use)
			SCR_FreeCoroutine(system, &system->coroutines[i]);
	}

	lua_close(system->lua);
	system->lua = NULL;

	DebugLogI(system->log_channel, "%s destroyed.", LUA_VERSION);
}

internal void
SCR_BindGlobal(SCR_System *system, String8 name, SCR_BindingFn *fn)
{
	SCR_BindGlobalWithUpval(system, name, fn, NULL, 0);
}

internal void
SCR_BindToTable(SCR_System *system, String8 table, String8 name, SCR_BindingFn *fn)
{
	SCR_BindToTableWithUpval(system, table, name, fn, NULL, 0);
}

internal void
SCR_BindGlobalWithUpval(SCR_System *system, String8 name,
						SCR_BindingFn *fn, void * const *upvalues, u32 n)
{
	SCR_PushBindingClosure(system, fn, upvalues, n);
	lua_setglobal(system->lua, (const char *)name.str);
}

internal void
SCR_BindToTableWithUpval(SCR_System *system, String8 table, String8 name,
						 SCR_BindingFn *fn, void * const *upvalues, u32 n)
{
	if (lua_getglobal(system->lua, (const char *)table.str) != LUA_TTABLE)
	{
		lua_pop(system->lua, 1);
		lua_newtable(system->lua);
		lua_pushvalue(system->lua, -1);
		lua_setglobal(system->lua, (const char *)table.str);
	}

	SCR_PushBindingClosure(system, fn, upvalues, n);
	lua_setfield(system->lua, -2, (const char *)name.str);
	lua_pop(system->lua, 1);
}

internal SCR_ScriptRef
SCR_Compile(SCR_System *system, IO_ByteSpan source, String8 chunk_name)
{
	SCR_ScriptRef result = SCR_ScriptRefNull();

	i32 status = luaL_loadbufferx(system->lua, (const char *)source.bytes, source.size, (const char *)chunk_name.str, "t");

	if (status != LUA_OK)
	{
		const char *err = lua_tostring(system->lua, -1);

		DebugLogE(system->log_channel,
				  "Compile failed (%.*s): %s",
				  String8VArg(chunk_name), err ? err : "(no message)");

		lua_pop(system->lua, 1);
	}
	else
	{
		result.value = luaL_ref(system->lua, LUA_REGISTRYINDEX);
	}

	return result;
}

internal void
SCR_Release(SCR_System *system, SCR_ScriptRef ref)
{
	if (SCR_ScriptRefIsNull(ref))
		return;

	luaL_unref(system->lua, LUA_REGISTRYINDEX, ref.value);
}

internal SCR_ScriptRef
SCR_ExecuteModule(SCR_System *system, SCR_ScriptRef chunk_ref)
{
	SCR_ScriptRef result = SCR_ScriptRefNull();

	if (SCR_ScriptRefIsNull(chunk_ref))
		goto end;

	lua_rawgeti(system->lua, LUA_REGISTRYINDEX, chunk_ref.value);
	
	if (lua_pcall(system->lua, 0, 1, 0) != LUA_OK)
	{
		DebugLogE(system->log_channel, "Module init error: %s", lua_tostring(system->lua, -1));

		lua_pop(system->lua, 1);
		goto end;
	}

	if (!lua_istable(system->lua, -1))
	{
		DebugLogE(system->log_channel, "Script did not return a table.");

		lua_pop(system->lua, 1);
		goto end;
	}

	result.value = luaL_ref(system->lua, LUA_REGISTRYINDEX);

end:
	return result;
}

internal SCR_ScriptRef
SCR_NewInstance(SCR_System *system, SCR_ScriptRef module_ref)
{
	SCR_ScriptRef result = SCR_ScriptRefNull();

	if (SCR_ScriptRefIsNull(module_ref))
		goto end;

	lua_newtable(system->lua); // instance
	lua_newtable(system->lua); // metatable
	lua_rawgeti(system->lua, LUA_REGISTRYINDEX, module_ref.value);
	lua_setfield(system->lua, -2, "__index");
	lua_setmetatable(system->lua, -2);
	
	result.value = luaL_ref(system->lua, LUA_REGISTRYINDEX);

end:
	return result;
}

internal SCR_Handle
SCR_PlayCallableOnThread(SCR_System *system, lua_State *thread, i32 thread_ref, u32 arg_count)
{
	SCR_Handle handle = {0};
	SCR_Coroutine *co = SCR_AllocCoroutine(system, &handle);

	if (!co)
	{
		luaL_unref(system->lua, LUA_REGISTRYINDEX, thread_ref);
		return SCR_HandleNull();
	}

	co->thread = thread;
	co->thread_ref = thread_ref;
	co->initial_arg_count = arg_count;
	co->yield_kind = SCR_YieldKind_Tick;
	co->has_been_resumed = false;

	return handle;
}

internal SCR_Handle
SCR_CallMethod(SCR_System *system, SCR_ScriptRef instance_ref, String8 method_name)
{
	return SCR_CallMethodEx(system, instance_ref, method_name, NULL, 0);
}

internal SCR_Handle
SCR_CallMethodEx(SCR_System *system, SCR_ScriptRef instance_ref, String8 method_name,
				 const SCR_Argument *args, u32 arg_count)
{
	if (SCR_ScriptRefIsNull(instance_ref))
		return SCR_HandleNull();

	lua_State *thread = lua_newthread(system->lua);
	i32 thread_ref = luaL_ref(system->lua, LUA_REGISTRYINDEX);

	lua_rawgeti(thread, LUA_REGISTRYINDEX, instance_ref.value); // self
	lua_getfield(thread, -1, (const char *)method_name.str);

	if (lua_type(thread, -1) != LUA_TFUNCTION)
	{
		DebugLogW(system->log_channel, "Method '%.*s' not found or not a function.", String8VArg(method_name));

		lua_settop(thread, 0);
		luaL_unref(system->lua, LUA_REGISTRYINDEX, thread_ref);
		return SCR_HandleNull();
	}

	// ok basically lua automaticlaly does this:
	// foo:Bar(...) becomes Bar(foo, ...)
	// i.e: object:Method() -> Method(object, ...)
	// so we gotta swap them before we call the method
	lua_insert(thread, -2);

	for (u32 i = 0; i < arg_count; i++)
		SCR_PushArg(thread, &args[i]);

	return SCR_PlayCallableOnThread(system, thread, thread_ref, arg_count + 1); // +1 bcuz of self
}

internal void
SCR_Stop(SCR_System *system, SCR_Handle handle)
{
	SCR_Coroutine *co = SCR_ResolveHandle(system, handle);

	if (!co)
		return;

	SCR_FreeCoroutine(system, co);
}

internal b32
SCR_IsRunning(const SCR_System *system, SCR_Handle handle)
{
	return SCR_ResolveHandle((SCR_System *)system, handle) != NULL;
}

internal void
SCR_SetOnFinish(SCR_System *system, SCR_Handle handle, SCR_FinishFn *fn, void *user_data)
{
	SCR_Coroutine *co = SCR_ResolveHandle(system, handle);

	if (!co)
		return;

	co->finish_fn = fn;
	co->finish_user_data = user_data;
}

internal void
SCR_FireSignal(SCR_System *system, String8 name)
{
	if (system->pending_signal_count >= SCR_MAX_PENDING_SIGNALS)
	{
		DebugLogE(system->log_channel,
				  "Signal queue full, dropping signal \"%.*s\".",
				  String8VArg(name));

		return;
	}

	u64 hash = HashStr8(name);
	system->pending_signals[system->pending_signal_count++] = hash;
}

internal b32
SCR_SignalIsPending(const SCR_System *system, u64 hash)
{
	for (u32 i = 0; i < system->pending_signal_count; i++)
	{
		if (system->pending_signals[i] == hash)
			return true;
	}

	return false;
}

internal b32
SCR_CoroutineIsReady(const SCR_System *system, const SCR_Coroutine *co)
{
	if (!co->in_use)
		return false;

	switch (co->yield_kind)
	{
		case SCR_YieldKind_Tick:
			return true;

		case SCR_YieldKind_TimeSeconds:
			return co->wait_remaining_s <= 0.f;

		case SCR_YieldKind_Signal:
			return SCR_SignalIsPending(system, co->wait_signal_hash);

		case SCR_YieldKind_Children:
			return co->active_children == 0;

		default:
			return false;
	}
}

internal void
SCR_ResumeOne(SCR_System *system, SCR_Coroutine *co)
{
	system->curr = co;

	i32 nargs = 0;
	
	if (!co->has_been_resumed)
	{
		nargs = co->initial_arg_count;
		co->has_been_resumed = true;
	}

	i32 nres = 0;
	i32 status = lua_resume(co->thread, system->lua, nargs, &nres);

	system->curr = NULL;

	if (status == LUA_OK)
	{
		if (nres > 0)
			lua_pop(co->thread, nres);
		
		SCR_FreeCoroutine(system, co);
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

		luaL_traceback(system->lua, co->thread, raw_msg, 0);
		const char *full_trace = lua_tostring(system->lua, -1);

		DebugLogE(system->log_channel, "%s", full_trace);

		lua_pop(system->lua, 1); // pop the traceback string from main state
		lua_pop(co->thread, 1);  // pop the original error object from the coroutine state

		SCR_FreeCoroutine(system, co);
	}
}

internal void
SCR_Tick(SCR_System *system, f32 dt)
{
	if (!system || !system->lua)
		return;

	for (u32 i = 1; i < SCR_MAX_COROUTINES; i++)
	{
		SCR_Coroutine *co = &system->coroutines[i];

		if (!co->in_use)
			continue;

		if (co->yield_kind == SCR_YieldKind_TimeSeconds)
		{
			co->wait_remaining_s -= dt;

			if (co->wait_remaining_s < 0.f)
				co->wait_remaining_s = 0.f;
		}
	}

	for (u32 i = 1; i < SCR_MAX_COROUTINES; i++)
	{
		SCR_Coroutine *co = &system->coroutines[i];

		if (!SCR_CoroutineIsReady(system, co))
			continue;

		SCR_ResumeOne(system, co);
	}

	system->pending_signal_count = 0;
}

internal u32
SCR_GetArgCount(SCR_Context *ctx)
{
	return lua_gettop(ctx->lua);
}

internal f32
SCR_ArgF32(SCR_Context *ctx, u32 idx)
{
	return luaL_checknumber(ctx->lua, idx + 1);
}

internal i32
SCR_ArgI32(SCR_Context *ctx, u32 idx)
{
	return luaL_checkinteger(ctx->lua, idx + 1);
}

internal b32
SCR_ArgBool(SCR_Context *ctx, u32 idx)
{
	luaL_checktype(ctx->lua, idx + 1, LUA_TBOOLEAN);
	return lua_toboolean(ctx->lua, idx + 1) ? 1 : 0;
}

internal String8
SCR_ArgString(SCR_Context *ctx, u32 idx)
{
	u64 len = 0;
	const char *s = luaL_checklstring(ctx->lua, idx + 1, &len);
	return String8Init(s, len);
}

internal u32
SCR_ArgTaggedU32(SCR_Context *ctx, u32 idx, u32 expected_tag)
{
	lua_Integer packed = luaL_checkinteger(ctx->lua, idx + 1);

	u32 tag   = (u32)(((u64)packed) >> 32);
	u32 value = (u32)(((u64)packed) & 0xFFFFFFFFu);

	if (tag != expected_tag)
	{
		luaL_error(ctx->lua,
				   "Argument %u: expected tag 0x%X, got 0x%X.",
				   idx, expected_tag, tag);
	}
	
	return value;
}

internal f32
SCR_ArgF32Opt(SCR_Context *ctx, u32 idx, f32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	return luaL_checknumber(ctx->lua, idx + 1);
}

internal i32
SCR_ArgI32Opt(SCR_Context *ctx, u32 idx, i32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	return luaL_checkinteger(ctx->lua, idx + 1);
}

internal b32
SCR_ArgB32Opt(SCR_Context *ctx, u32 idx, b32 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	luaL_checktype(ctx->lua, idx + 1, LUA_TBOOLEAN);
	return lua_toboolean(ctx->lua, idx + 1) ? 1 : 0;
}

internal String8
SCR_ArgStringOpt(SCR_Context *ctx, u32 idx, String8 fallback)
{
	if (lua_isnoneornil(ctx->lua, idx + 1))
		return fallback;

	u64 len = 0;
	const char *s = luaL_checklstring(ctx->lua, idx + 1, &len);

	return String8Init(s, len);
}

internal void *
SCR_UpvaluePtr(SCR_Context *ctx, u32 idx)
{
	i32 up_index = lua_upvalueindex(idx + SCR_USER_UPVAL_BASE);
	return lua_touserdata(ctx->lua, up_index);
}

internal void
SCR_ReturnNil(SCR_Context *ctx)
{
	lua_pushnil(ctx->lua);
	ctx->nretval++;
}

internal void
SCR_ReturnF32(SCR_Context *ctx, f32 v)
{
	lua_pushnumber(ctx->lua, v);
	ctx->nretval++;
}

internal void
SCR_ReturnI32(SCR_Context *ctx, i32 v)
{
	lua_pushinteger(ctx->lua, v);
	ctx->nretval++;
}

internal void
SCR_ReturnB32(SCR_Context *ctx, b32 v)
{
	lua_pushboolean(ctx->lua, v ? 1 : 0);
	ctx->nretval++;
}

internal void
SCR_ReturnString8(SCR_Context *ctx, String8 v)
{
	lua_pushlstring(ctx->lua, (const char *)v.str, v.len);
	ctx->nretval++;
}

internal void
SCR_ReturnTaggedU32(SCR_Context *ctx, u32 v, u32 type_tag)
{
	lua_Integer packed = (lua_Integer)(((u64)type_tag << 32) | (u64)v);
	lua_pushinteger(ctx->lua, packed);
	ctx->nretval++;
}

internal void
SCR_Yield(SCR_Context *ctx)
{
	SCR_Coroutine *co = ctx->system->curr;

	DebugLogAssert(ctx->system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(ctx->system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = SCR_YieldKind_Tick;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = 0;

	lua_yield(ctx->lua, 0);
}

internal void
SCR_YieldTime(SCR_Context *ctx, f32 time_s)
{
	SCR_Coroutine *co = ctx->system->curr;

	DebugLogAssert(ctx->system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(ctx->system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = SCR_YieldKind_TimeSeconds;
	co->wait_remaining_s = time_s;
	co->wait_signal_hash = 0;

	lua_yield(ctx->lua, 0);
}

internal void
SCR_YieldSignal(SCR_Context *ctx, String8 name)
{
	SCR_Coroutine *co = ctx->system->curr;

	DebugLogAssert(ctx->system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(ctx->system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = SCR_YieldKind_Signal;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = HashStr8(name);

	lua_yield(ctx->lua, 0);
}

internal i32
SCR_ContinuationTrampoline(lua_State *lua, i32 status, lua_KContext kctx)
{
	SCR_System *sys = SCR_GetSystemFromL(lua);
	SCR_Coroutine *co  = sys->curr;

	SCR_ContinueFn *cont = co->continue_fn;
	void *user_data = co->continue_user_data;

	co->continue_fn = NULL;
	co->continue_user_data = NULL;

	SCR_Context ctx = {0};
	ctx.system = sys;
	ctx.lua = lua;
	ctx.nretval = 0;

	if (cont)
		cont(&ctx, user_data);

	return ctx.nretval;
}

internal void
SCR_YieldSignalCont(SCR_Context *ctx, String8 name, SCR_ContinueFn *cont, void *user_data)
{
	SCR_Coroutine *co = ctx->system->curr;

	DebugLogAssert(ctx->system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(ctx->system->log_channel, ctx->lua == co->thread, "Nested coroutines.");

	co->yield_kind = SCR_YieldKind_Signal;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = HashStr8(name);
	co->continue_fn = cont;
	co->continue_user_data = user_data;

	lua_yieldk(ctx->lua, 0, 0, SCR_ContinuationTrampoline);
}

internal void
SCR_Throw(SCR_Context *ctx, const char *fmt, ...)
{
	char buf[512] = {0};

	va_list param;
	va_start(param, fmt);
	vsnprintf(buf, sizeof(buf), fmt, param);
	va_end(param);

	luaL_error(ctx->lua, "%s", buf);
}

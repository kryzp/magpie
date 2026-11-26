#include "script/system.h"
#include "core/hash.h"

#include <cstdarg>
#include <cstdlib>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

// lua indices start at 1
// first index reserved for function pointer
// start from 2
#define SCRIPT_USER_UPVAL_BASE 2

static scr::System *selected_system = nullptr;

static void *InternalLuaAllocator(void *ud, void *ptr, u64 old_size, u64 new_size)
{
	if (new_size == 0) {
		free(ptr);
		return nullptr;
	}

	return realloc(ptr, new_size);
}

static void PushArg(lua_State *target, const scr::Argument &a)
{
	switch (a.type) {
		case scr::ArgType_Nil:
			lua_pushnil(target);
			break;

		case scr::ArgType_Float:
			lua_pushnumber(target, a.float_value);
			break;

		case scr::ArgType_Int:
			lua_pushinteger(target, a.int_value);
			break;

		case scr::ArgType_Bool:
			lua_pushboolean(target, a.bool_value ? 1 : 0);
			break;

		case scr::ArgType_String:
			lua_pushlstring(target, a.string_value.data(), a.string_value.size());
			break;

		case scr::ArgType_TaggedU32:
			lua_pushinteger(target, static_cast<lua_Integer>(scr::PackTaggedU32(a.tagged_value, a.tag)));
			break;

		default:
			lua_pushnil(target);
			break;
	}
}

scr::System *scr::System::Get()
{
	return selected_system;
}

scr::System::System(LogChannel log_channel_)
	: log_channel(log_channel_)
	, lua(nullptr)
	, first_free_hint(0)
	, curr(nullptr)
	, pending_signal_count(0)
{
	lua = lua_newstate(InternalLuaAllocator, nullptr, 0);

	DebugLogAssert(log_channel, lua, "Lua failed to initialize (still NULL).");

	luaL_openlibs(lua);

	for (int i = 0; i < SCRIPT_MAX_COROUTINES; i++) {
		coroutines[i].in_use = false;
		coroutines[i].generation = 1; // 0 = null
		coroutines[i].thread_ref = LUA_NOREF;
		coroutines[i].parent = -1;
	}

	selected_system = this;

	BindGlobal("dispatch", BindDispatch);
	BindGlobal("parallelize", BindParallelize);

	DebugLogI(log_channel, "Initialized using %s.", LUA_VERSION);
}

scr::System::~System()
{
	if (lua) {
		for (int i = 0; i < SCRIPT_MAX_COROUTINES; i++) {
			if (coroutines[i].in_use)
				FreeCoroutine(&coroutines[i]);
		}

		lua_close(lua);
		lua = nullptr;
	}

	DebugLogI(log_channel, "Destroyed.");

	selected_system = nullptr;
}

scr::Coroutine *scr::System::AllocCoroutine(CoroutineHandle *out_handle)
{
	// 0 = null
	for (int i = 1; i < SCRIPT_MAX_COROUTINES; i++) {
		int slot = (first_free_hint + i) % SCRIPT_MAX_COROUTINES;

		if (slot == 0)
			continue;

		Coroutine *co = &coroutines[slot];

		if (!co->in_use) {
			co->in_use = true;
			co->has_been_resumed = false;
			co->initial_arg_count = 0;

			co->yield_kind = YieldKind_Tick;
			co->wait_remaining_s = 0.f;
			co->wait_signal_hash = 0;
			co->parent = -1;
			co->active_children = 0;
			co->continue_fn = nullptr;
			co->continue_user_data = nullptr;
			co->finish_fn = nullptr;
			co->finish_user_data = nullptr;
			co->thread = nullptr;
			co->thread_ref = LUA_NOREF;

			first_free_hint = slot;

			if (out_handle) {
				out_handle->index = slot;
				out_handle->generation = co->generation;
			}

			return co;
		}
	}

	DebugLogE(log_channel, "Out of coroutine slots (max %u).", SCRIPT_MAX_COROUTINES);

	if (out_handle)
		*out_handle = CoroutineHandle();

	return nullptr;
}

void scr::System::FreeCoroutine(Coroutine *co)
{
	if (co->finish_fn)
		co->finish_fn(co, co->finish_user_data);

	if (co->parent >= 0 && co->parent < SCRIPT_MAX_COROUTINES) {
		Coroutine *parent = &coroutines[co->parent];

		if (parent->in_use && parent->active_children > 0)
			parent->active_children--;
	}

	if (co->thread_ref != LUA_NOREF) {
		luaL_unref(lua, LUA_REGISTRYINDEX, co->thread_ref);
		co->thread_ref = LUA_NOREF;
	}

	co->thread = nullptr;
	co->in_use = false;
	co->generation++;
	co->yield_kind = YieldKind_None;
}

scr::Coroutine *scr::System::ResolveHandle(CoroutineHandle handle)
{
	if (handle.IsNull())
		return nullptr;

	if (handle.index == 0 || handle.index >= SCRIPT_MAX_COROUTINES)
		return nullptr;

	Coroutine *co = &coroutines[handle.index];

	if (!co->in_use)
		return nullptr;

	if (co->generation != handle.generation)
		return nullptr;

	return co;
}

int scr::System::BindingTrampoline(lua_State *lua)
{
	BindingFn *fn = reinterpret_cast<BindingFn *>(lua_touserdata(lua, lua_upvalueindex(1)));

	Context ctx(lua);

	if (fn)
		fn(&ctx);

	return ctx.nretval;
}

void scr::System::PushBindingClosure(BindingFn *fn, void *const *upvalues, unsigned n)
{
	// first upval is reserved for the function pointer
	lua_pushlightuserdata(lua, reinterpret_cast<void *>(fn));

	for (int i = 0; i < n; i++)
		lua_pushlightuserdata(lua, upvalues ? upvalues[i] : nullptr);

	lua_pushcclosure(lua, BindingTrampoline, SCRIPT_USER_UPVAL_BASE - 1 + n);
}

void scr::System::BindDispatch(Context *ctx)
{
	System *self = selected_system;

	unsigned n = ctx->GetArgCount();

	if (n == 0) {
		ctx->Throw("dispatch() has no parameters.");
		return;
	}

	Coroutine *parent = self->curr;

	if (!parent) {
		ctx->Throw("dispatch() can only be called from inside a coroutine.");
		return;
	}

	CoroutineHandle child_handle;
	Coroutine *child = self->AllocCoroutine(&child_handle);

	if (!child) {
		DebugLogE(self->log_channel, "Coroutine pool exhausted.");
		return;
	}

	child->thread = lua_newthread(self->lua);
	child->thread_ref = luaL_ref(self->lua, LUA_REGISTRYINDEX);

	// push function and arguments onto new thread stack
	for (int i = 1; i <= n; i++)
		lua_pushvalue(ctx->lua, i);

	lua_xmove(ctx->lua, child->thread, n);

	child->has_been_resumed = false;
	child->initial_arg_count = n;
	child->yield_kind = YieldKind_Tick;
	child->parent = -1;
}

void scr::System::BindParallelize(Context *ctx)
{
	System *self = selected_system;

	unsigned n = ctx->GetArgCount();

	if (n == 0) {
		ctx->Throw("parallelize() has no parameters.");
		return;
	}

	Coroutine *parent = self->curr;

	if (!parent) {
		ctx->Throw("parallelize() can only be called from inside a coroutine.");
		return;
	}

	parent->active_children = n;

	const int parent_idx = static_cast<int>(parent - self->coroutines);

	for (int i = 0; i < n; i++) {
		CoroutineHandle child_handle;
		Coroutine *child = self->AllocCoroutine(&child_handle);

		if (!child) {
			parent->active_children--;
			DebugLogE(self->log_channel, "Coroutine pool exhausted.");
			continue;
		}

		child->thread = lua_newthread(self->lua);
		child->thread_ref = luaL_ref(self->lua, LUA_REGISTRYINDEX);

		lua_pushvalue(ctx->lua, i + 1);
		lua_xmove(ctx->lua, child->thread, 1);

		child->has_been_resumed = false;
		child->initial_arg_count = 0;
		child->yield_kind = YieldKind_Tick;
		child->parent = parent_idx;
	}

	parent->yield_kind = YieldKind_Children;

	lua_yield(ctx->lua, 0);
}

scr::Ref scr::System::Compile(const void *source, u64 size, const String &chunk_name)
{
	Ref result;

	int status = luaL_loadbufferx(lua, static_cast<const char *>(source), size, chunk_name.c_str(), "t");

	if (status != LUA_OK) {
		const char *err = lua_tostring(lua, -1);

		DebugLogE(
			log_channel,
			"Compile failed (%s): %s",
			chunk_name.c_str(), err ? err : "(no message)"
		);

		lua_pop(lua, 1);
	} else {
		result.value = luaL_ref(lua, LUA_REGISTRYINDEX);
	}

	return result;
}

void scr::System::Release(Ref ref)
{
	if (ref.IsNull())
		return;

	luaL_unref(lua, LUA_REGISTRYINDEX, ref.value);
}

scr::Ref scr::System::ExecuteModule(Ref chunk_ref)
{
	Ref result;

	if (chunk_ref.IsNull())
		return result;

	lua_rawgeti(lua, LUA_REGISTRYINDEX, chunk_ref.value);

	if (lua_pcall(lua, 0, 1, 0) != LUA_OK) {
		DebugLogE(log_channel, "Module init error: %s", lua_tostring(lua, -1));
		lua_pop(lua, 1);
		return result;
	}

	if (!lua_istable(lua, -1)) {
		DebugLogE(log_channel, "Script did not return a table.");
		lua_pop(lua, 1);
		return result;
	}

	result.value = luaL_ref(lua, LUA_REGISTRYINDEX);

	return result;
}

scr::Ref scr::System::NewInstance(Ref module_ref)
{
	Ref result;

	if (module_ref.IsNull())
		return result;

	lua_newtable(lua); // instance
	lua_newtable(lua); // metatable

	lua_rawgeti(lua, LUA_REGISTRYINDEX, module_ref.value);
	lua_setfield(lua, -2, "__index");

	lua_setmetatable(lua, -2);

	result.value = luaL_ref(lua, LUA_REGISTRYINDEX);

	return result;
}

scr::CoroutineHandle scr::System::PlayCallableOnThread(lua_State *thread, int thread_ref, unsigned arg_count)
{
	CoroutineHandle handle;
	Coroutine *co = AllocCoroutine(&handle);

	if (!co) {
		luaL_unref(lua, LUA_REGISTRYINDEX, thread_ref);
		return CoroutineHandle();
	}

	co->thread = thread;
	co->thread_ref = thread_ref;
	co->initial_arg_count = arg_count;
	co->yield_kind = YieldKind_Tick;
	co->has_been_resumed = false;

	return handle;
}

scr::CoroutineHandle scr::System::CallMethod(Ref instance_ref, const String &method_name)
{
	return CallMethodEx(instance_ref, method_name, nullptr, 0);
}

scr::CoroutineHandle scr::System::CallMethodEx(Ref instance_ref, const String &method_name, const Argument *args, unsigned arg_count)
{
	if (instance_ref.IsNull())
		return CoroutineHandle();

	lua_State *thread = lua_newthread(lua);
	int thread_ref = luaL_ref(lua, LUA_REGISTRYINDEX);

	lua_rawgeti(thread, LUA_REGISTRYINDEX, instance_ref.value); // self
	lua_getfield(thread, -1, method_name.c_str());

	if (lua_type(thread, -1) != LUA_TFUNCTION) {
		DebugLogW(log_channel, "Method '%s' not found or not a function.", method_name.c_str());

		lua_settop(thread, 0);
		luaL_unref(lua, LUA_REGISTRYINDEX, thread_ref);

		return CoroutineHandle();
	}

	// ok basically lua automaticlaly does this:
	// foo:Bar(...) becomes Bar(foo, ...)
	// i.e: object:Method() -> Method(object, ...)
	// so we gotta swap them before we call the method
	lua_insert(thread, -2);

	for (int i = 0; i < arg_count; i++)
		PushArg(thread, args[i]);

	return PlayCallableOnThread(thread, thread_ref, arg_count + 1); // +1 bcuz of self
}

void scr::System::Stop(CoroutineHandle handle)
{
	Coroutine *co = ResolveHandle(handle);

	if (!co)
		return;

	FreeCoroutine(co);
}

bool scr::System::IsRunning(CoroutineHandle handle)
{
	return ResolveHandle(handle) != nullptr;
}

void scr::System::FireSignal(const String &name)
{
	if (pending_signal_count >= SCRIPT_MAX_PENDING_SIGNALS) {
		DebugLogE(
			log_channel,
			"Signal queue full, dropping signal \"%s\".",
			name.c_str()
		);

		return;
	}

	pending_signals[pending_signal_count++] = hash::Str(name);
}

bool scr::System::SignalIsPending(u64 hash) const
{
	for (int i = 0; i < pending_signal_count; i++) {
		if (pending_signals[i] == hash)
			return true;
	}

	return false;
}

bool scr::System::CoroutineIsReady(const Coroutine *co) const
{
	if (!co->in_use)
		return false;

	switch (co->yield_kind) {
		case YieldKind_Tick:
			return true;

		case YieldKind_TimeSeconds:
			return co->wait_remaining_s <= 0.f;

		case YieldKind_Signal:
			return SignalIsPending(co->wait_signal_hash);

		case YieldKind_Children:
			return co->active_children == 0;

		default:
			return false;
	}
}

void scr::System::ResumeOne(Coroutine *co)
{
	curr = co;

	int nargs = 0;

	if (!co->has_been_resumed) {
		nargs = co->initial_arg_count;
		co->has_been_resumed = true;
	}

	int nres = 0;
	int status = lua_resume(co->thread, lua, nargs, &nres);

	curr = nullptr;

	if (status == LUA_OK) {
		if (nres > 0)
			lua_pop(co->thread, nres);

		FreeCoroutine(co);
	} else if (status == LUA_YIELD) {
		if (nres > 0)
			lua_pop(co->thread, nres);
	} else {
		const char *raw_msg = lua_tostring(co->thread, -1);

		if (!raw_msg)
			raw_msg = "(no message)";

		luaL_traceback(lua, co->thread, raw_msg, 0);

		const char *full_trace = lua_tostring(lua, -1);

		DebugLogE(log_channel, "%s", full_trace);

		lua_pop(lua, 1);        // pop the traceback string from main state
		lua_pop(co->thread, 1); // pop the original error object from the coroutine state

		FreeCoroutine(co);
	}
}

void scr::System::Tick(float dt)
{
	if (!lua)
		return;

	for (int i = 1; i < SCRIPT_MAX_COROUTINES; i++) {
		Coroutine *co = &coroutines[i];

		if (!co->in_use)
			continue;

		if (co->yield_kind == YieldKind_TimeSeconds) {
			co->wait_remaining_s -= dt;

			if (co->wait_remaining_s < 0.f)
				co->wait_remaining_s = 0.f;
		}
	}

	for (int i = 1; i < SCRIPT_MAX_COROUTINES; i++) {
		Coroutine *co = &coroutines[i];

		if (!CoroutineIsReady(co))
			continue;

		ResumeOne(co);
	}

	pending_signal_count = 0;
}

void scr::System::SetOnFinish(CoroutineHandle handle, FinishFn *fn, void *user_data)
{
	Coroutine *co = ResolveHandle(handle);

	if (!co)
		return;

	co->finish_fn = fn;
	co->finish_user_data = user_data;
}

void scr::System::BindGlobal(const String &name, BindingFn *fn)
{
	BindGlobalEx(name, fn, nullptr, 0);
}

void scr::System::BindToTable(const String &table, const String &name, BindingFn *fn)
{
	BindToTableEx(table, name, fn, nullptr, 0);
}

void scr::System::BindGlobalEx(const String &name, BindingFn *fn, void *const *upvalues, unsigned n)
{
	PushBindingClosure(fn, upvalues, n);
	lua_setglobal(lua, name.c_str());
}

void scr::System::BindToTableEx(const String &table, const String &name, BindingFn *fn, void *const *upvalues, unsigned n)
{
	if (lua_getglobal(lua, table.c_str()) != LUA_TTABLE) {
		lua_pop(lua, 1);
		lua_newtable(lua);
		lua_pushvalue(lua, -1);
		lua_setglobal(lua, table.c_str());
	}

	PushBindingClosure(fn, upvalues, n);
	lua_setfield(lua, -2, name.c_str());
	lua_pop(lua, 1);
}

unsigned scr::Context::GetArgCount()
{
	return static_cast<unsigned>(lua_gettop(lua));
}

float scr::Context::GetArgFloat(unsigned idx)
{
	return static_cast<float>(luaL_checknumber(lua, idx + 1));
}

int scr::Context::GetArgInt(unsigned idx)
{
	return static_cast<int>(luaL_checkinteger(lua, idx + 1));
}

bool scr::Context::GetArgBool(unsigned idx)
{
	luaL_checktype(lua, idx + 1, LUA_TBOOLEAN);
	return lua_toboolean(lua, idx + 1) != 0;
}

String scr::Context::GetArgString(unsigned idx)
{
	u64 len = 0;
	const char *s = luaL_checklstring(lua, idx + 1, &len);
	return String(s, len);
}

unsigned scr::Context::GetArgTaggedU32(unsigned idx, unsigned expected_tag)
{
	lua_Integer packed = luaL_checkinteger(lua, idx + 1);

	unsigned value = 0;
	unsigned tag = 0;

	UnpackTaggedU32(packed, &value, &tag);

	if (tag != expected_tag) {
		luaL_error(
			lua,
			"Argument %u: Expected tag 0x%X, got 0x%X.",
			idx, expected_tag, tag
		);
	}

	return value;
}

float scr::Context::GetArgFloatOpt(unsigned idx, float fallback)
{
	if (lua_isnoneornil(lua, idx + 1))
		return fallback;

	return static_cast<float>(luaL_checknumber(lua, idx + 1));
}

int scr::Context::GetArgIntOpt(unsigned idx, int fallback)
{
	if (lua_isnoneornil(lua, idx + 1))
		return fallback;

	return static_cast<int>(luaL_checkinteger(lua, idx + 1));
}

bool scr::Context::GetArgBoolOpt(unsigned idx, bool fallback)
{
	if (lua_isnoneornil(lua, idx + 1))
		return fallback;

	luaL_checktype(lua, idx + 1, LUA_TBOOLEAN);
	return lua_toboolean(lua, idx + 1) != 0;
}

String scr::Context::GetArgStringOpt(unsigned idx, const String &fallback)
{
	if (lua_isnoneornil(lua, idx + 1))
		return fallback;

	u64 len = 0;
	const char *s = luaL_checklstring(lua, idx + 1, &len);
	return String(s, len);
}

void *scr::Context::UpvaluePtr(unsigned idx)
{
	int up_index = lua_upvalueindex(static_cast<int>(idx) + SCRIPT_USER_UPVAL_BASE);
	return lua_touserdata(lua, up_index);
}

void scr::Context::ReturnNil()
{
	lua_pushnil(lua);
	nretval++;
}

void scr::Context::ReturnFloat(float v)
{
	lua_pushnumber(lua, v);
	nretval++;
}

void scr::Context::ReturnInt(int v)
{
	lua_pushinteger(lua, v);
	nretval++;
}

void scr::Context::ReturnBool(bool v)
{
	lua_pushboolean(lua, v ? 1 : 0);
	nretval++;
}

void scr::Context::ReturnString(const String &v)
{
	lua_pushlstring(lua, v.data(), v.size());
	nretval++;
}

void scr::Context::ReturnTaggedU32(unsigned v, unsigned type_tag)
{
	lua_pushinteger(lua, static_cast<lua_Integer>(PackTaggedU32(v, type_tag)));
	nretval++;
}

void scr::Context::Yield()
{
	System *system = selected_system;
	Coroutine *co = system->curr;

	DebugLogAssert(system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(system->log_channel, lua == co->thread, "Nested coroutines.");

	co->yield_kind = YieldKind_Tick;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = 0;

	lua_yield(lua, 0);
}

void scr::Context::YieldTime(float time_s)
{
	System *system = selected_system;
	Coroutine *co = system->curr;

	DebugLogAssert(system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(system->log_channel, lua == co->thread, "Nested coroutines.");

	co->yield_kind = YieldKind_TimeSeconds;
	co->wait_remaining_s = time_s;
	co->wait_signal_hash = 0;

	lua_yield(lua, 0);
}

void scr::Context::YieldSignal(const String &name)
{
	System *system = selected_system;
	Coroutine *co = system->curr;

	DebugLogAssert(system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(system->log_channel, lua == co->thread, "Nested coroutines.");

	co->yield_kind = YieldKind_Signal;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = hash::Str(name);

	lua_yield(lua, 0);
}

int scr::System::ContinuationTrampoline(lua_State *lua, int status, intptr_t kctx)
{
	(void)status;
	(void)kctx;

	Coroutine *co = selected_system->curr;

	ContinueFn *cont = co->continue_fn;
	void *user_data = co->continue_user_data;

	co->continue_fn = nullptr;
	co->continue_user_data = nullptr;

	Context ctx(lua);

	if (cont)
		cont(&ctx, user_data);

	return ctx.nretval;
}

void scr::Context::YieldSignalCont(const String &name, ContinueFn *cont, void *user_data)
{
	System *system = selected_system;
	Coroutine *co = system->curr;

	DebugLogAssert(system->log_channel, co, "Can only yield from inside a coroutine.");
	DebugLogAssert(system->log_channel, lua == co->thread, "Nested coroutines.");

	co->yield_kind = YieldKind_Signal;
	co->wait_remaining_s = 0.f;
	co->wait_signal_hash = hash::Str(name);
	co->continue_fn = cont;
	co->continue_user_data = user_data;

	lua_yieldk(lua, 0, 0, System::ContinuationTrampoline);
}

void scr::Context::Throw(const char *fmt, ...)
{
	char buf[512] = {};

	va_list param;
	va_start(param, fmt);
	vsnprintf(buf, sizeof(buf), fmt, param);
	va_end(param);

	luaL_error(lua, "%s", buf);
}

#ifndef SCRIPT_SYSTEM_H
#define SCRIPT_SYSTEM_H

#include "core/types.h"
#include "core/log.h"
#include "script/handle.h"
#include "script/argument.h"

#include "container/string.h"

#define SCRIPT_MAX_COROUTINES      512
#define SCRIPT_MAX_PENDING_SIGNALS 512

struct lua_State;

namespace scr {

class Context;
struct Coroutine;

typedef void BindingFn(Context *ctx);
typedef void ContinueFn(Context *ctx, void *user_data);
typedef void FinishFn(Coroutine *co, void *user_data);

enum YieldKind {
	YieldKind_None,
	YieldKind_Tick, // . . . . . . resume on the next tick
	YieldKind_TimeSeconds, //  . . resume after wait_remaining_s reaches zero
	YieldKind_Signal, // . . . . . resume when wait_signal_hash fires
	YieldKind_Children, // . . . . resume when active_children reaches zero
	YieldKind_MaxEnum
};

struct Coroutine {
	bool in_use = false;
	unsigned generation = 1; // 0 = null

	lua_State *thread = nullptr;
	int thread_ref = -2; // NOT AN OS THREAD. luaL_ref into LUA_REGISTRYINDEX

	bool has_been_resumed = false;
	unsigned initial_arg_count = 0;

	YieldKind yield_kind = YieldKind_None;

	float wait_remaining_s = 0.f;
	u64 wait_signal_hash = 0;

	int parent = -1; // -1 = none
	unsigned active_children = 0;

	ContinueFn *continue_fn = nullptr;
	void *continue_user_data = nullptr;

	FinishFn *finish_fn = nullptr;
	void *finish_user_data = nullptr;
};

class Context {
public:
	explicit Context(lua_State *lua_)
		: lua(lua_)
		, nretval(0)
	{
	}

	unsigned GetArgCount();
	float GetArgFloat(unsigned idx);
	int GetArgInt(unsigned idx);
	bool GetArgBool(unsigned idx);
	String GetArgString(unsigned idx);
	unsigned GetArgTaggedU32(unsigned idx, unsigned expected_tag);

	float GetArgFloatOpt(unsigned idx, float fallback);
	int GetArgIntOpt(unsigned idx, int fallback);
	bool GetArgBoolOpt(unsigned idx, bool fallback);
	String GetArgStringOpt(unsigned idx, const String &fallback);

	void *UpvaluePtr(unsigned idx);

	void ReturnNil();
	void ReturnFloat(float v);
	void ReturnInt(int v);
	void ReturnBool(bool v);
	void ReturnString(const String &v);
	void ReturnTaggedU32(unsigned v, unsigned type_tag);

	void Yield();
	void YieldTime(float time_s);
	void YieldSignal(const String &name);
	void YieldSignalCont(const String &name, ContinueFn *cont, void *user_data);

	void Throw(const char *fmt, ...);

	lua_State *lua;
	int nretval;
};

class System {
public:
	explicit System(LogChannel log_channel);
	~System();

	System(const System &) = delete;
	System &operator=(const System &) = delete;

	static System *Get();

	void Tick(float dt);

	Ref Compile(const void *source, u64 size, const String &chunk_name);
	void Release(Ref ref);

	Ref ExecuteModule(Ref chunk_ref);
	Ref NewInstance(Ref module_ref);

	CoroutineHandle CallMethod(Ref instance_ref, const String &method_name);
	CoroutineHandle CallMethodEx(Ref instance_ref, const String &method_name, const Argument *args, unsigned arg_count);

	void Stop(CoroutineHandle handle);
	bool IsRunning(CoroutineHandle handle);

	void FireSignal(const String &name);

	void SetOnFinish(CoroutineHandle handle, FinishFn *fn, void *user_data);

	void BindGlobal(const String &name, BindingFn *fn);
	void BindToTable(const String &table, const String &name, BindingFn *fn);
	void BindGlobalEx(const String &name, BindingFn *fn, void *const *upvalues, unsigned n);
	void BindToTableEx(const String &table, const String &name, BindingFn *fn, void *const *upvalues, unsigned n);

	LogChannel GetLogChannel() const
	{
		return log_channel;
	}

private:
	friend class Context;

	Coroutine *AllocCoroutine(CoroutineHandle *out_handle);
	void FreeCoroutine(Coroutine *co);
	Coroutine *ResolveHandle(CoroutineHandle handle);

	void PushBindingClosure(BindingFn *fn, void *const *upvalues, unsigned n);

	CoroutineHandle PlayCallableOnThread(lua_State *thread, int thread_ref, unsigned arg_count);

	bool SignalIsPending(u64 hash) const;
	bool CoroutineIsReady(const Coroutine *co) const;
	void ResumeOne(Coroutine *co);

	static void BindDispatch(Context *ctx);
	static void BindParallelize(Context *ctx);

	static int BindingTrampoline(lua_State *lua);
	static int ContinuationTrampoline(lua_State *lua, int status, intptr_t kctx);

	LogChannel log_channel;

	lua_State *lua;

	Coroutine coroutines[SCRIPT_MAX_COROUTINES];
	unsigned first_free_hint;

	Coroutine *curr; // set for the duration of a resume

	u64 pending_signals[SCRIPT_MAX_PENDING_SIGNALS];
	unsigned pending_signal_count;
};

}

#endif // SCRIPT_SYSTEM_H

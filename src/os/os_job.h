#ifndef OS_J_H
#define OS_J_H

#define J_ENTRY_POINT_SIG(fn) void fn(void *param)
#define J_ENTRY_POINT_DEF(fn) internal J_ENTRY_POINT_SIG(fn)

#define J_PARALLEL_FOR_SIG(fn) void fn(u32 index)
#define J_PARALLEL_FOR_DEF(fn) internal J_PARALLEL_FOR_SIG(fn)

typedef J_ENTRY_POINT_SIG(J_EntryPointFn);
typedef J_PARALLEL_FOR_SIG(J_EntryForFn);

typedef enum J_Priority
{
	J_Priority_Low,
	J_Priority_Normal,
	J_Priority_High,
	J_Priority_COUNT
}
J_Priority;

typedef u32 J_Flags;
enum
{
	J_Flag_None           = 0,
	J_Flag_MainThreadOnly = 1 << 0
};

typedef struct J_Decl J_Decl;
struct J_Decl
{
	J_EntryPointFn *EntryPoint;
	void *param;
	J_Priority priority;
	J_Flags flags;
};

#endif // OS_J_H

#ifndef OS_JOB_H
#define OS_JOB_H

#define JOB_ENTRY_POINT_SIG(fn) void fn(void *param)
#define JOB_ENTRY_POINT_DEF(fn) internal JOB_ENTRY_POINT_SIG(fn)

#define JOB_PARALLEL_FOR_SIG(fn) void fn(u32 index)
#define JOB_PARALLEL_FOR_DEF(fn) internal JOB_PARALLEL_FOR_SIG(fn)

typedef JOB_ENTRY_POINT_SIG(JOB_EntryPointFn);
typedef JOB_PARALLEL_FOR_SIG(JOB_EntryForFn);

typedef enum JOB_Priority
{
	JOB_Priority_Low,
	JOB_Priority_Normal,
	JOB_Priority_High,
	JOB_Priority_COUNT
}
JOB_Priority;

typedef u32 JOB_Flags;
enum
{
	JOB_Flag_None           = 0,
	JOB_Flag_MainThreadOnly = 1 << 0
};

typedef struct JOB_Decl JOB_Decl;
struct JOB_Decl
{
	JOB_EntryPointFn *EntryPoint;
	void *param;
	JOB_Priority priority;
	JOB_Flags flags;
};

#endif // OS_JOB_H

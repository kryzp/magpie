#ifndef ASSET_STATE_H
#define ASSET_STATE_H

typedef enum A_State
{
	A_State_Unloaded,
	A_State_CpuStage,
	A_State_WaitingForDependencies,
	A_State_GpuStage,
	A_State_Ready,
	A_State_Failed,
	A_State_COUNT
}
A_State;

/*
 * TODO: Get rid of these functions just do clear checks in the asset manager this is just making code harder.
 */

static inline b32 A_StateIsLoading(A_State st)
{
	return
		st == A_State_CpuStage ||
		st == A_State_WaitingForDependencies ||
		st == A_State_GpuStage;
}

static inline b32 A_StateNeedsLoad(A_State st)
{
	return
		st == A_State_Unloaded ||
		st == A_State_Failed;
}

static inline b32 A_StateIsLoaded(A_State st)
{
	return
		st == A_State_Ready;
}

static inline b32 A_StateIsFinalized(A_State st)
{
	return
		st == A_State_Ready ||
		st == A_State_Failed;
}

#endif // ASSET_STATE_H

#ifndef ASSET_STATE_H
#define ASSET_STATE_H

typedef enum AST_State
{
	AST_State_Unloaded,
	AST_State_CpuStage,
	AST_State_WaitingForDependencies,
	AST_State_GpuStage,
	AST_State_Ready,
	AST_State_Failed,
	AST_State_COUNT
}
AST_State;

/*
 * TODO: Get rid of these functions just do clear checks in the asset manager this is just making code harder.
 */

internal inline b32
AST_StateIsLoading(AST_State st)
{
	return
		st == AST_State_CpuStage ||
		st == AST_State_WaitingForDependencies ||
		st == AST_State_GpuStage;
}

internal inline b32
AST_StateNeedsLoad(AST_State st)
{
	return
		st == AST_State_Unloaded ||
		st == AST_State_Failed;
}

internal inline b32
AST_StateIsLoaded(AST_State st)
{
	return
		st == AST_State_Ready;
}

internal inline b32
AST_StateIsFinalized(AST_State st)
{
	return
		st == AST_State_Ready ||
		st == AST_State_Failed;
}

#endif // ASSET_STATE_H

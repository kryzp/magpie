#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

typedef struct AN_Handle AN_Handle;
struct AN_Handle
{
	u32 index;
	u32 generation;
};

internal inline AN_Handle AN_HandleNull(void)
{
	AN_Handle handle = {0};
	handle.index = (u32)-1;
	handle.generation = 0;
	
	return handle;
}

internal inline b32 AN_HandleIsNull(AN_Handle handle)
{
	return (handle.index == (u32)-1 &&
			handle.generation == 0);
}

internal inline b32 AN_HandleMatch(AN_Handle a, AN_Handle b)
{
	return (a.index == b.index &&
			a.generation == b.generation);
}

typedef struct AN_Instance AN_Instance;
struct AN_Instance
{
	Arena arena;
	
	AN_Animator animator;
	
	b32 alive;
	u32 generation;
};

typedef struct AN_System AN_System;
struct AN_System
{
	LOG_Channel log_channel;

	AN_Instance instances[128]; // todo: make dynamically allocated
	u32 instance_count;

	u32 free_indices[128];
	u32 free_index_count;
};

internal void AN_SystemInitAndSelect(AN_System *system, LOG_Channel log_channel);
internal void AN_SystemDestroy(void);
internal void AN_SystemSelectContext(AN_System *system);

internal void AN_SystemCalculateIntermediatePoses(f32 elapsed);
internal void AN_SystemFinalizePoseAndMatrixPalette(void);

internal AN_Instance *AN_SystemResolve(AN_Handle handle);
internal AN_Animator *AN_SystemGetAnimator(AN_Handle handle);

internal AN_Palette AN_GetPalette(AN_Handle handle, i32 skin_index);

internal AN_Handle AN_SystemCreateInstance(A_Handle model_handle);
internal void AN_SystemKillInstance(AN_Handle h);

#endif // ANIMATION_SYSTEM_H

#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

typedef struct AN_Handle AN_Handle;
struct AN_Handle
{
	u32 index;
	u32 generation;
};

static inline AN_Handle AN_HandleNull(void)
{
	AN_Handle handle = {0};
	handle.index = (u32)-1;
	handle.generation = 0;
	
	return handle;
}

static inline b32 AN_HandleMatch(AN_Handle a, AN_Handle b)
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

static void AN_SystemInit(AN_System *s, LOG_Channel log_channel, A_Assets *assets);
static void AN_SystemDestroy(AN_System *s);

static void AN_SystemCalculateIntermediatePoses(AN_System *s, f32 elapsed);
static void AN_SystemFinalizePoseAndMatrixPalette(AN_System *s);

static AN_Instance *AN_SystemResolve(AN_System *s, AN_Handle handle);
static AN_Animator *AN_SystemGetAnimator(AN_System *s, AN_Handle handle);

static void AN_Play(AN_System *s, AN_Handle handle, AN_ClipKey clip, b32 loop, f32 global_start_time);
static b32 AN_IsFinished(AN_System *s, AN_Handle handle);
static AN_Palette AN_GetPalette(AN_System *s, AN_Handle handle, i32 skin_index);

static AN_Handle AN_SystemCreateInstance(AN_System *s, A_Handle model_handle);
static void AN_SystemKillInstance(AN_System *s, AN_Handle h);

#endif // ANIMATION_SYSTEM_H

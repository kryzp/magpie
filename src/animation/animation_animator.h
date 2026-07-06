#ifndef ANIMATION_ANIMATOR_H
#define ANIMATION_ANIMATOR_H

typedef struct AN_Palette AN_Palette;
struct AN_Palette
{
	const m4 *matrices;
	u32 joint_count;
};

typedef struct AN_JointPose AN_JointPose;
struct AN_JointPose
{
	v3 translation;
	v4 rotation;
	v3 scale;
};

typedef struct AN_InterpolatedKeyframe AN_InterpolatedKeyframe;
struct AN_InterpolatedKeyframe
{
	u32 k0;
	u32 k1;
	f32 progress;
};

typedef struct AN_SkeletonPose AN_SkeletonPose;
struct AN_SkeletonPose
{
	u32 joint_count;
	
	AN_JointPose *local_poses;
	
	m4 *global_transforms; // hierarchy accumulated

	m4 *palette;
};

typedef struct AN_ClipKey AN_ClipKey;
struct AN_ClipKey
{
	i32 value;
};

static AN_ClipKey AN_ClipKeyNull(void)
{
	AN_ClipKey null_key = {0};
	null_key.value = -1;

	return null_key;
}

static b32 AN_ClipKeyIsNull(AN_ClipKey k)
{
	return k.value < 0;
}

typedef struct AN_Animator AN_Animator;
struct AN_Animator
{
	A_Handle selected_model;

	AN_ClipKey clip;
	b32 loop;
	f32 global_start_time;
	b32 paused;
	f32 global_paused_time;
	f32 playback_rate;

	u32 pose_count;
	AN_SkeletonPose *poses;

	f32 last_sample_time;
};


/* ==================================================
   HELPERS
   ================================================== */

static m4 AN_JointPoseToM4(AN_JointPose trs);
static AN_JointPose AN_JointPoseBlend(AN_JointPose a, AN_JointPose b, f32 u);

static f32 AN_TimestampProgressFactor(f32 prev_ts, f32 next_ts, f32 ts);
static AN_InterpolatedKeyframe AN_InterpolateKeyframe(const A_AnimChannel *ch, f32 ts);
static void AN_SampleChannel(const A_AnimChannel *ch, f32 ts, AN_JointPose *local_trs);

static f32 AN_CalcSampleTime(f32 global_time, f32 global_start_time, f32 playback_rate, f32 duration, u32 n);


/* ==================================================
   ANIMATOR
   ================================================== */

static void AN_AnimatorSelect(AN_Animator *animator, Arena *arena, A_Handle model_handle);
static void AN_AnimatorTick(AN_Animator *animator, f32 global_time);
static void AN_AnimatorUpdatePalette(AN_Animator *animator);

static AN_ClipKey AN_AnimatorFindClipByName(AN_Animator *animator, String8 name);

static void AN_AnimatorPlay(AN_Animator *animator, AN_ClipKey clip, b32 loop, f32 global_start_time);
static void AN_AnimatorStop(AN_Animator *animator);

static void AN_AnimatorResume(AN_Animator *animator);
static void AN_AnimatorPause(AN_Animator *animator, f32 global_time);
static void AN_AnimatorPauseAndReset(AN_Animator *animator, f32 global_time);

static b32 AN_AnimatorIsPlaying(const AN_Animator *animator, AN_ClipKey clip);
static b32 AN_AnimatorIsFinished(const AN_Animator *animator);
static f32 AN_AnimatorCalcNormalizedTimeForCurrentClip(const AN_Animator *animator);

static AN_Palette AN_AnimatorPalette(AN_Animator *animator, i32 skin_index);


#endif // ANIMATION_ANIMATOR_H

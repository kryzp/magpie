#ifndef ANIMATION_ANIMATOR_H
#define ANIMATION_ANIMATOR_H

#define AN_CLIP_INVALID ((u32)-1)

typedef struct AN_Palette AN_Palette;
struct AN_Palette
{
	const m4 *palette;
	u32 joint_count;
};

typedef struct AN_TRS AN_TRS;
struct AN_TRS
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
	
	AN_TRS *curr_local_transforms;
	AN_TRS *prev_local_transforms;
	
	m4 *global_transforms; // hierarchy accumulated

	m4 *palette;
};

typedef struct AN_Animator AN_Animator;
struct AN_Animator
{
	A_Handle selected_model;

	u32 curr_clip;
	f32 curr_elapsed;
	b32 curr_finished;

	u32 prev_clip;
	f32 prev_elapsed;

	f32 playback_rate;
	b32 loop;

	f32 blend_elapsed;
	f32 blend_duration;
	
	u32 pose_count;
	AN_SkeletonPose *poses;
};


/* ==================================================
   HELPERS
   ================================================== */

static m4 AN_TRSToM4(AN_TRS trs);
static AN_TRS AN_TRSBlend(AN_TRS a, AN_TRS b, f32 u);

static f32 AN_TimestampProgressFactor(f32 prev_ts, f32 next_ts, f32 ts);
static AN_InterpolatedKeyframe AN_InterpolateKeyframe(const A_AnimChannel *ch, f32 ts);
static void AN_SampleChannel(const A_AnimChannel *ch, f32 ts, AN_TRS *local_trs);


/* ==================================================
   ANIMATOR
   ================================================== */

static void       AN_AnimatorSelect            (AN_Animator *animator, Arena *arena, A_Registry *assets, A_Handle model_handle);
static void       AN_AnimatorTick              (AN_Animator *animator, A_Registry *assets, f32 dt);

static void       AN_AnimatorPlay              (AN_Animator *animator, u32 clip, b32 loop);
static b32        AN_AnimatorPlayByName        (AN_Animator *animator, A_Registry *assets, String8 name, b32 loop);

static void       AN_AnimatorCrossFadeTo       (AN_Animator *animator, u32 clip, b32 loop, f32 blend_duration);
static b32        AN_AnimatorCrossFadeToByName (AN_Animator *animator, A_Registry *assets, String8 name, b32 loop, f32 blend_duration);

static b32        AN_AnimatorFinished          (const AN_Animator *animator);

static AN_Palette AN_AnimatorPalette           (AN_Animator *animator, i32 skin_index);


#endif // ANIMATION_ANIMATOR_H

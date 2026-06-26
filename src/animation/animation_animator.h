#ifndef ANIMATION_ANIMATOR_H
#define ANIMATION_ANIMATOR_H

#define AN_CLIP_INVALID ((u32)-1)

typedef struct AN_Palette AN_Palette;
struct AN_Palette
{
	const m4 *palette;
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

typedef struct AN_Animator AN_Animator;
struct AN_Animator
{
	A_Handle selected_model;

	u32 clip;
	b32 loop;
	f32 global_start_time;
	f32 playback_rate;

	u32 pose_count;
	AN_SkeletonPose *poses;
};


/* ==================================================
   HELPERS
   ================================================== */

static m4 AN_JointPoseToM4(AN_JointPose trs);
static AN_JointPose AN_JointPoseBlend(AN_JointPose a, AN_JointPose b, f32 u);

static f32 AN_TimestampProgressFactor(f32 prev_ts, f32 next_ts, f32 ts);
static AN_InterpolatedKeyframe AN_InterpolateKeyframe(const A_AnimChannel *ch, f32 ts);
static void AN_SampleChannel(const A_AnimChannel *ch, f32 ts, AN_JointPose *local_trs);

static f32 AN_GetSampleTime(f32 global_time, f32 global_start_time, f32 playback_rate, f32 duration, u32 n);

/* ==================================================
   ANIMATOR
   ================================================== */

static void       AN_AnimatorSelect            (AN_Animator *animator, Arena *arena, A_Assets *assets, A_Handle model_handle);
static void       AN_AnimatorTick              (AN_Animator *animator, A_Assets *assets, f32 global_time);
static void       AN_AnimatorUpdatePalette     (AN_Animator *animator, A_Assets *assets);

static void       AN_AnimatorPlay              (AN_Animator *animator, u32 clip, b32 loop, f32 global_start_time);
static b32        AN_AnimatorPlayByName        (AN_Animator *animator, A_Assets *assets, String8 name, b32 loop, f32 global_start_time);

static AN_Palette AN_AnimatorPalette           (AN_Animator *animator, i32 skin_index);


#endif // ANIMATION_ANIMATOR_H

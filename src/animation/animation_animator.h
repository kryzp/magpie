#ifndef ANIMATION_ANIMATOR_H
#define ANIMATION_ANIMATOR_H

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
	
	AN_TRS *local_transforms;
	m4 *global_transforms; // hierarchy accumulated

	m4 *palette;
};

typedef struct AN_Animator AN_Animator;
struct AN_Animator
{
	A_Handle selected_model;

	u32 active_clip;

	f32 elapsed;
	f32 playback_rate;

	b32 loop;

	u32 pose_count;
	AN_SkeletonPose *poses;
};


/* ==================================================
   HELPERS
   ================================================== */

internal m4 AN_TRSToM4(AN_TRS trs);
internal f32 AN_TimestampProgressFactor(f32 prev_ts, f32 next_ts, f32 ts);
internal AN_InterpolatedKeyframe AN_InterpolateKeyframe(const A_AnimChannel *ch, f32 ts);
internal void AN_SampleChannel(const A_AnimChannel *ch, f32 ts, AN_TRS *local_trs);


/* ==================================================
   ANIMATOR
   ================================================== */

internal void         AN_AnimatorSelect     (AN_Animator *animator, Arena *arena, A_Registry *assets, A_Handle model_handle);
internal void         AN_AnimatorTick       (AN_Animator *animator, A_Registry *assets, f32 dt);
internal void         AN_AnimatorPlay       (AN_Animator *animator, u32 clip);
internal b32          AN_AnimatorPlayByName (AN_Animator *animator, A_Registry *assets, String8 name);
internal AN_Palette AN_AnimatorPalette    (AN_Animator *animator, i32 skin_index);


#endif // ANIMATION_ANIMATOR_H

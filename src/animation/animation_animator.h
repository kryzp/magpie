#ifndef ANIMATION_ANIMATOR_H
#define ANIMATION_ANIMATOR_H

typedef struct ANIM_Palette ANIM_Palette;
struct ANIM_Palette
{
	const m4 *palette;
	u32 joint_count;
};

typedef struct ANIM_TRS ANIM_TRS;
struct ANIM_TRS
{
	v3 translation;
	v4 rotation;
	v3 scale;
};

typedef struct ANIM_SkeletonPose ANIM_SkeletonPose;
struct ANIM_SkeletonPose
{
	u32 joint_count;
	
	ANIM_TRS *local_transforms;
	m4 *global_transforms; // hierarchy accumulated

	m4 *palette;
};

typedef struct ANIM_Animator ANIM_Animator;
struct ANIM_Animator
{
	AST_Handle selected_model;

	u32 active_clip;

	f32 elapsed;
	f32 playback_rate;

	b32 loop;

	u32 pose_count;
	ANIM_SkeletonPose *poses;
};


/* ==================================================
   HELPERS
   ================================================== */

internal m4           ANIM_TRSToM4(ANIM_TRS trs);
internal f32          ANIM_ScaleFactor(f32 last_ts, f32 next_ts, f32 t);
internal void         ANIM_FindKeyframes(const AST_AnimChannel *ch, f32 t, u32 *out_k0, u32 *out_k1, f32 *out_u);
internal void         ANIM_SampleChannel(const AST_AnimChannel *ch, f32 t, ANIM_TRS *local_trs);


/* ==================================================
   CORE
   ================================================== */

internal void         ANIM_AnimatorSelect     (ANIM_Animator *animator, Arena *arena, AST_Assets *assets, AST_Handle model_handle);
internal void         ANIM_AnimatorTick       (ANIM_Animator *animator, AST_Assets *assets, f32 dt);


/* ==================================================
   CONTROL
   ================================================== */

internal void         ANIM_AnimatorPlay       (ANIM_Animator *animator, u32 clip);
internal b32          ANIM_AnimatorPlayByName (ANIM_Animator *animator, AST_Assets *assets, String8 name);


/* ==================================================
   PALETTE
   ================================================== */

internal ANIM_Palette ANIM_AnimatorPalette    (ANIM_Animator *animator, i32 skin_index);


#endif // ANIMATION_ANIMATOR_H

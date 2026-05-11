
internal m4
ANIM_TRSToM4(ANIM_TRS trs)
{
	m4 T = M4Translate(trs.translation);
	m4 R = M4RotateQuat(trs.rotation);
	m4 S = M4Scale(trs.scale);

	return M4MulM4(T, M4MulM4(R, S));
}

internal f32
ANIM_ScaleFactor(f32 last_ts, f32 next_ts, f32 t)
{
	if (next_ts <= last_ts)
		return 0.f;

	return (t - last_ts) / (next_ts - last_ts);
}

internal void
ANIM_FindKeyframes(const AST_AnimChannel *ch, f32 t, u32 *out_k0, u32 *out_k1, f32 *out_u)
{
	AssertTrue(out_k0 && out_k1 && out_u);
	
	if (ch->key_count == 1 || t <= ch->keys[0].timestamp_s)
	{
		*out_k0 = 0;
		*out_k1 = 0;
		*out_u  = 0.f;

		return;
	}

	if (t >= ch->keys[ch->key_count - 1].timestamp_s)
	{
		*out_k0 = ch->key_count - 1;
		*out_k1 = ch->key_count - 1;
		*out_u  = 0.f;

		return;
	}

	for (u32 i = 0; i < ch->key_count - 1; i++)
	{
		if (t >= ch->keys[i + 1].timestamp_s)
			continue;

		*out_k0 = i;
		*out_k1 = i + 1;
		*out_u  = ANIM_ScaleFactor(ch->keys[i].timestamp_s, ch->keys[i + 1].timestamp_s, t);

		return;
	}

	AssertTrue(false && "shouldn't get here anyway something's gone wrong");
	
	*out_k0 = 0;
	*out_k1 = 0;
	*out_u  = 0;
}

internal void
ANIM_SampleChannel(const AST_AnimChannel *ch, f32 t, ANIM_TRS *local_trs)
{
	u32 k0 = 0;
	u32 k1 = 0;
	f32 u  = 0.f;

	ANIM_FindKeyframes(ch, t, &k0, &k1, &u);

	b32 step = ch->interp == AST_AnimInterp_Step;

	switch (ch->path)
	{
		case AST_AnimPath_Translate:
		{
			local_trs->translation = step
				? ch->keys[k0].translation
				: V3Lerp(ch->keys[k0].translation, ch->keys[k1].translation, u);
		}
		break;
		
		case AST_AnimPath_Rotation:
		{
			local_trs->rotation = step
				? ch->keys[k0].rotation
				: V4QuatSlerp(ch->keys[k0].rotation, ch->keys[k1].rotation, u);
		}
		break;
		
		case AST_AnimPath_Scale:
		{
			local_trs->scale = step
				? ch->keys[k0].scale
				: V3Lerp(ch->keys[k0].scale, ch->keys[k1].scale, u);
		}
		break;
	}
}

internal void
ANIM_AnimatorSelect(ANIM_Animator *animator, Arena *arena, AST_Assets *assets, AST_Handle model_handle)
{
	AST_Asset *asset = AST_GetNow(assets, model_handle, AST_Type_Model);

	animator->selected_model = model_handle;
	animator->active_clip = (u32)-1;
	animator->elapsed = 0;
	animator->playback_rate = 1.f;
	animator->loop = true;

	animator->pose_count = asset->model.skeleton_count;
	animator->poses = ArenaPushArray(arena, ANIM_SkeletonPose, animator->pose_count);

	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const AST_Skeleton *skeleton = &asset->model.skeletons[i];

		u32 count = skeleton->joint_count;

		animator->poses[i].joint_count       = count;
		animator->poses[i].local_transforms  = ArenaPushArray(arena, ANIM_TRS, count);
		animator->poses[i].global_transforms = ArenaPushArray(arena, m4,       count);
		animator->poses[i].palette           = ArenaPushArray(arena, m4,       count);
	}
}

internal void
ANIM_AnimatorTick(ANIM_Animator *animator, AST_Assets *assets, f32 dt)
{
	if (animator->pose_count <= 0)
		return;
	
	AST_Asset *asset = AST_Get(assets, animator->selected_model, AST_Type_Model);

	if (!asset)
		return;

	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const AST_Skeleton *skeleton = &asset->model.skeletons[i];

		for (u32 j = 0; j < skeleton->joint_count; j++)
		{
			animator->poses[i].local_transforms[j].translation = skeleton->joints[j].bind_translate;
			animator->poses[i].local_transforms[j].rotation    = skeleton->joints[j].bind_rotation;
			animator->poses[i].local_transforms[j].scale       = skeleton->joints[j].bind_scale;
		}
	}

	if (animator->active_clip < asset->model.clip_count)
	{
		AST_AnimClip *clip = &asset->model.clips[animator->active_clip];

		animator->elapsed += dt * animator->playback_rate;

		if (clip->duration_s > 0.f)
		{
			if (animator->loop)
			{
				while (animator->elapsed > clip->duration_s)
					animator->elapsed -= clip->duration_s;
			}
			else
			{
				animator->elapsed = ClampValue(animator->elapsed, 0.f, clip->duration_s);
			}
		}

		for (u32 i = 0; i < clip->channel_count; i++)
		{
			const AST_AnimChannel *ch = &clip->channels[i];

			if (ch->target_skeleton < 0 || ch->target_skeleton > animator->pose_count)
				continue;

			ANIM_SkeletonPose *pose = &animator->poses[ch->target_skeleton];

			if (ch->target_joint >= pose->joint_count)
				continue;

			ANIM_SampleChannel(ch, animator->elapsed, &pose->local_transforms[ch->target_joint]);
		}
	}

	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const AST_Skeleton *skeleton = &asset->model.skeletons[i];
		ANIM_SkeletonPose *pose = &animator->poses[i];
		
		for (u32 j = 0; j < skeleton->joint_count; j++)
		{
			m4 local_transform = ANIM_TRSToM4(pose->local_transforms[j]);
			i32 parent = skeleton->joints[j].parent;

			pose->global_transforms[j] = parent < 0
				? local_transform
				: M4MulM4(pose->global_transforms[parent], local_transform);

			pose->palette[j] = M4MulM4(pose->global_transforms[j], skeleton->joints[j].inverse_bind_matrix);
		}
	}
}

internal void
ANIM_AnimatorPlay(ANIM_Animator *animator, u32 clip)
{
	animator->active_clip = clip;
	animator->elapsed = 0.f;
}

internal b32
ANIM_AnimatorPlayByName(ANIM_Animator *animator, AST_Assets *assets, String8 name)
{
	AST_Asset *asset = AST_Get(assets, animator->selected_model, AST_Type_Model);

	if (!asset)
		return false;

	for (u32 i = 0; i < asset->model.clip_count; i++)
	{
		if (String8Match(asset->model.clips[i].name, name))
		{
			ANIM_AnimatorPlay(animator, i);
			return true;
		}
	}

	return false;
}

internal ANIM_Palette
ANIM_AnimatorPalette(ANIM_Animator *animator, i32 skin_index)
{
	ANIM_Palette palette = {0};

	if (skin_index < 0 || skin_index >= animator->pose_count)
		return palette;

	palette.palette     = animator->poses[skin_index].palette;
	palette.joint_count = animator->poses[skin_index].joint_count;

	return palette;
}


internal m4
ANIM_TRSToM4(ANIM_TRS trs)
{
	m4 T = M4Translate(trs.translation);
	m4 R = M4RotateQuat(trs.rotation);
	m4 S = M4Scale(trs.scale);

	return M4MulM4(T, M4MulM4(R, S));
}

internal f32
ANIM_TimestampProgressFactor(f32 prev_ts, f32 next_ts, f32 ts)
{
	if (next_ts <= prev_ts)
		return 0.f;

	return (ts - prev_ts) / (next_ts - prev_ts);
}

internal ANIM_InterpolatedKeyframe
ANIM_InterpolateKeyframe(const AST_AnimChannel *ch, f32 ts)
{
	ANIM_InterpolatedKeyframe keyframe = {0};
	
	if (ch->key_count == 1 || ts <= ch->keys[0].timestamp_s)
	{
		return keyframe;
	}

	if (ts >= ch->keys[ch->key_count - 1].timestamp_s)
	{
		keyframe.k0 = ch->key_count - 1;
		keyframe.k1 = ch->key_count - 1;
		keyframe.progress = 0.f;

		return keyframe;
	}

	for (u32 i = 0; i < ch->key_count - 1; i++)
	{
		if (ts >= ch->keys[i + 1].timestamp_s)
			continue;

		keyframe.k0 = i + 0;
		keyframe.k1 = i + 1;

		f32 prev_ts = ch->keys[keyframe.k0].timestamp_s;
		f32 next_ts = ch->keys[keyframe.k1].timestamp_s;
		
		keyframe.progress = ANIM_TimestampProgressFactor(prev_ts, next_ts, ts);

		return keyframe;
	}

	AssertTrue(false);

	return keyframe;
}

internal void
ANIM_SampleChannel(const AST_AnimChannel *ch, f32 ts, ANIM_TRS *local_trs)
{
	ANIM_InterpolatedKeyframe keyframe = ANIM_InterpolateKeyframe(ch, ts);

	const AST_AnimKey *k0 = &ch->keys[keyframe.k0];
	const AST_AnimKey *k1 = &ch->keys[keyframe.k1];
	
	f32 progress = keyframe.progress;

	if (ch->interp == AST_AnimInterp_Step)
	{
		// Discrete Interpolation.
	
		switch (ch->path)
		{
			case AST_AnimPath_Translate:
				local_trs->translation = k0->translation;
				break;
		
			case AST_AnimPath_Rotation:
				local_trs->rotation = k0->rotation;
				break;
		
			case AST_AnimPath_Scale:
				local_trs->scale = k0->scale;
				break;
		}
	}
	else if (ch->interp == AST_AnimInterp_Linear)
	{
		// Linear Interpoaltion.
	
		switch (ch->path)
		{
			case AST_AnimPath_Translate:
				local_trs->translation = V3Lerp(k0->translation,
											 k1->translation,
											 progress);
				break;
		
			case AST_AnimPath_Rotation:
				local_trs->rotation = V4QuatSlerp(k0->rotation,
											   k1->rotation,
											   progress);
				break;
		
			case AST_AnimPath_Scale:
				local_trs->scale = V3Lerp(k0->scale,
									   k1->scale,
									   progress);
				break;
		}
	}
	else if (ch->interp == AST_AnimInterp_Cubic)
	{
		AssertTrue(false);
	}
}

internal void
ANIM_AnimatorSelect(ANIM_Animator *animator, Arena *arena, AST_Assets *assets, AST_Handle model_handle)
{
	AST_Asset *asset = AST_GetNow(assets, model_handle, AST_Type_Model);

	AssertTrue(asset);
	
	AST_AssetModel *asset_model = &asset->model_data;

	animator->selected_model = model_handle;
	animator->active_clip = (u32)-1;
	animator->elapsed = 0;
	animator->playback_rate = 1.f;
	animator->loop = true;

	animator->pose_count = asset_model->skeleton_count;
	animator->poses = ArenaPushArray(arena, ANIM_SkeletonPose, animator->pose_count);

	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const AST_Skeleton *skeleton = &asset_model->skeletons[i];

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
	
	AST_AssetModel *asset_model = &AST_Get(assets, animator->selected_model, AST_Type_Model)->model_data;

	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const AST_Skeleton *skeleton = &asset_model->skeletons[i];

		for (u32 j = 0; j < skeleton->joint_count; j++)
		{
			animator->poses[i].local_transforms[j].translation = skeleton->joints[j].bind_translation;
			animator->poses[i].local_transforms[j].rotation    = skeleton->joints[j].bind_rotation;
			animator->poses[i].local_transforms[j].scale       = skeleton->joints[j].bind_scale;
		}
	}

	if (animator->active_clip < asset_model->clip_count)
	{
		AST_AnimClip *clip = &asset_model->clips[animator->active_clip];

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

			if (ch->target_skeleton < 0 || ch->target_skeleton >= animator->pose_count)
				continue;

			ANIM_SkeletonPose *pose = &animator->poses[ch->target_skeleton];

			if (ch->target_joint >= pose->joint_count)
				continue;

			ANIM_SampleChannel(ch, animator->elapsed, &pose->local_transforms[ch->target_joint]);
		}
	}

	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const AST_Skeleton *skeleton = &asset_model->skeletons[i];
		ANIM_SkeletonPose *pose = &animator->poses[i];
		
		for (u32 j = 0; j < skeleton->joint_count; j++)
		{
			m4 local_transform = ANIM_TRSToM4(pose->local_transforms[j]);
			i32 parent = skeleton->joints[j].parent;

			pose->global_transforms[j] = parent < 0
				? M4MulM4(skeleton->root_parent_world, local_transform)
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
	AST_AssetModel *asset_model = &AST_Get(assets, animator->selected_model, AST_Type_Model)->model_data;

	for (u32 i = 0; i < asset_model->clip_count; i++)
	{
		if (String8Match(asset_model->clips[i].name, name))
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

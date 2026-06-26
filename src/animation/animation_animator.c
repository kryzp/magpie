
static m4 AN_JointPoseToM4(AN_JointPose trs)
{
	m4 T = M4Translate(trs.translation);
	m4 R = M4RotateQuat(trs.rotation);
	m4 S = M4Scale(trs.scale);

	return M4MulM4(T, M4MulM4(R, S));
}

static AN_JointPose AN_JointPoseBlend(AN_JointPose a, AN_JointPose b, f32 u)
{
	AN_JointPose blended = {0};

	blended.translation = V3Lerp(a.translation, b.translation, u);
	blended.rotation = V4QuatSlerp(a.rotation, b.rotation, u);
	blended.scale = V3Lerp(a.scale, b.scale, u);

	return blended;
}

static f32 AN_TimestampProgressFactor(f32 prev_ts, f32 next_ts, f32 ts)
{
	if (next_ts <= prev_ts)
		return 0.f;

	return (ts - prev_ts) / (next_ts - prev_ts);
}

static AN_InterpolatedKeyframe AN_InterpolateKeyframe(const A_AnimChannel *ch, f32 ts)
{
	AN_InterpolatedKeyframe keyframe = {0};
	
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
		
		keyframe.progress = AN_TimestampProgressFactor(prev_ts, next_ts, ts);

		return keyframe;
	}

	AssertTrue(false);

	return keyframe;
}

static void AN_SampleChannel(const A_AnimChannel *ch, f32 ts, AN_JointPose *local_trs)
{
	AN_InterpolatedKeyframe keyframe = AN_InterpolateKeyframe(ch, ts);

	const A_AnimKey *k0 = &ch->keys[keyframe.k0];
	const A_AnimKey *k1 = &ch->keys[keyframe.k1];
	
	f32 progress = keyframe.progress;

	if (ch->interp == A_AnimInterp_Step)
	{
		switch (ch->path)
		{
			case A_AnimPath_Translate:
				local_trs->translation = k0->translation;
				break;
		
			case A_AnimPath_Rotation:
				local_trs->rotation = k0->rotation;
				break;
		
			case A_AnimPath_Scale:
				local_trs->scale = k0->scale;
				break;
		}
	}
	else if (ch->interp == A_AnimInterp_Linear)
	{
		switch (ch->path)
		{
			case A_AnimPath_Translate:
				local_trs->translation = V3Lerp(k0->translation,
												k1->translation,
												progress);
				break;
		
			case A_AnimPath_Rotation:
				local_trs->rotation = V4QuatSlerp(k0->rotation,
												  k1->rotation,
												  progress);
				break;
		
			case A_AnimPath_Scale:
				local_trs->scale = V3Lerp(k0->scale,
										  k1->scale,
										  progress);
				break;
		}
	}
	else if (ch->interp == A_AnimInterp_Cubic)
	{
		AssertTrue(false);
	}
}

static f32 AN_GetSampleTime(f32 global_time, f32 global_start_time, f32 playback_rate, f32 duration, u32 n)
{
	f32 sample_time = (global_time - global_start_time) * playback_rate;

	if (n > 0)
		sample_time = ClampValue(sample_time, 0.f, duration * (f32)n);
	
	while (sample_time >= duration)
		sample_time -= duration;

	return sample_time;
}

static void AN_AnimatorSelect(AN_Animator *animator, Arena *arena, A_Assets *assets, A_Handle model_handle)
{
	A_Asset *asset = A_GetNow(assets, model_handle);

	AssertTrue(asset);
	
	A_ModelData *asset_model = &asset->model;

	animator->selected_model = model_handle;
	
	animator->clip = AN_CLIP_INVALID;
	
	animator->playback_rate = 1.f;
	animator->loop = true;

	animator->pose_count = asset_model->skeleton_count;
	animator->poses = ArenaPushArray(arena, AN_SkeletonPose, animator->pose_count);

	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const A_Skeleton *skeleton = &asset_model->skeletons[i];

		u32 count = skeleton->joint_count;

		animator->poses[i].joint_count       = count;
		animator->poses[i].local_poses       = ArenaPushArray(arena, AN_JointPose, count);
		animator->poses[i].global_transforms = ArenaPushArray(arena, m4,     count);
		animator->poses[i].palette           = ArenaPushArray(arena, m4,     count);
	}
}

static void AN_AnimatorTick(AN_Animator *animator, A_Assets *assets, f32 elapsed)
{
	if (animator->pose_count <= 0)
		return;
	
	A_ModelData *asset_model = &A_Get(assets, animator->selected_model)->model;

	// reset bind pose
	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const A_Skeleton *skeleton = &asset_model->skeletons[i];

		for (u32 j = 0; j < skeleton->joint_count; j++)
		{
			animator->poses[i].local_poses[j].translation = skeleton->joints[j].bind_translation;
			animator->poses[i].local_poses[j].rotation = skeleton->joints[j].bind_rotation;
			animator->poses[i].local_poses[j].scale = skeleton->joints[j].bind_scale;
		}
	}

	// advance and sample the clip
	if (animator->clip != AN_CLIP_INVALID)
	{
		if (animator->clip < asset_model->clip_count)
		{
			A_AnimClip *clip = &asset_model->clips[animator->clip];

			f32 sample_time = AN_GetSampleTime(elapsed, animator->global_start_time, animator->playback_rate, clip->duration_s, 0);

			for (u32 i = 0; i < clip->channel_count; i++)
			{
				const A_AnimChannel *ch = &clip->channels[i];

				if (ch->target_skeleton < 0 || ch->target_skeleton >= animator->pose_count)
					continue;

				AN_SkeletonPose *pose = &animator->poses[ch->target_skeleton];

				if (ch->target_joint >= pose->joint_count)
					continue;

				AN_SampleChannel(ch, sample_time, &pose->local_poses[ch->target_joint]);
			}
		}
	}
}

static void AN_AnimatorUpdatePalette(AN_Animator *animator, A_Assets *assets)
{
	if (animator->pose_count <= 0)
		return;
	
	A_ModelData *asset_model = &A_Get(assets, animator->selected_model)->model;
	
	for (u32 i = 0; i < animator->pose_count; i++)
	{
		const A_Skeleton *skeleton = &asset_model->skeletons[i];
		AN_SkeletonPose *pose = &animator->poses[i];
		
		for (u32 j = 0; j < skeleton->joint_count; j++)
		{
			m4 local_transform = AN_JointPoseToM4(pose->local_poses[j]);
			i32 parent = skeleton->joints[j].parent;

			pose->global_transforms[j] = parent < 0
				? M4MulM4(skeleton->root_parent_world, local_transform)
				: M4MulM4(pose->global_transforms[parent], local_transform);

			pose->palette[j] = M4MulM4(pose->global_transforms[j], skeleton->joints[j].inverse_bind_matrix);
		}
	}
}

static void AN_AnimatorPlay(AN_Animator *animator, u32 clip, b32 loop, f32 global_start_time)
{
	if (clip == animator->clip &&
		loop == animator->loop)
		return;

	animator->clip = clip;
	animator->loop = loop;
	animator->global_start_time = global_start_time;
}

static b32 AN_AnimatorPlayByName(AN_Animator *animator, A_Assets *assets, String8 name, b32 loop, f32 global_start_time)
{
	A_ModelData *asset_model = &A_Get(assets, animator->selected_model)->model;

	for (u32 i = 0; i < asset_model->clip_count; i++)
	{
		if (String8Match(asset_model->clips[i].name, name))
		{
			AN_AnimatorPlay(animator, i, loop, global_start_time);
			return true;
		}
	}

	return false;
}

static AN_Palette AN_AnimatorPalette(AN_Animator *animator, i32 skin_index)
{
	AN_Palette palette = {0};

	if (skin_index < 0 || skin_index >= animator->pose_count)
		return palette;

	palette.palette     = animator->poses[skin_index].palette;
	palette.joint_count = animator->poses[skin_index].joint_count;

	return palette;
}

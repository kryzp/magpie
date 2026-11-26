#include "animation/animator.h"
#include "asset/asset_system.h"
#include "core/log.h"

Mat4 anim::JointPose::AsMatrix() const
{
	Mat4 T = Mat4::Translate(translation);
	Mat4 R = Mat4::RotateQuat(rotation);
	Mat4 S = Mat4::Scale(scale);

	return T * (R * S);
}

anim::JointPose anim::JointPose::Blend(const JointPose &other, float u) const
{
	JointPose blended;
	blended.translation = translation.Lerp(other.translation, u);
	blended.rotation = rotation.Slerp(other.rotation, u);
	blended.scale = scale.Lerp(other.scale, u);

	return blended;
}

float anim::TimestampProgressFactor(float prev_ts, float next_ts, float ts)
{
	if (next_ts <= prev_ts)
		return 0.f;

	return (ts - prev_ts) / (next_ts - prev_ts);
}

anim::InterpolatedKeyframe anim::InterpolateKeyframe(const ast::AnimChannel &ch, float ts)
{
	InterpolatedKeyframe keyframe;

	unsigned key_count = ch.keys.size();

	if (key_count == 1 || ts <= ch.keys[0].timestamp_s)
		return keyframe;

	if (ts >= ch.keys[key_count - 1].timestamp_s) {
		keyframe.k0 = key_count - 1;
		keyframe.k1 = key_count - 1;
		keyframe.progress = 0.f;

		return keyframe;
	}

	for (int i = 0; i < key_count - 1; i++) {
		if (ts >= ch.keys[i + 1].timestamp_s)
			continue;

		keyframe.k0 = i + 0;
		keyframe.k1 = i + 1;

		float prev_ts = ch.keys[keyframe.k0].timestamp_s;
		float next_ts = ch.keys[keyframe.k1].timestamp_s;

		keyframe.progress = TimestampProgressFactor(prev_ts, next_ts, ts);

		return keyframe;
	}

	AssertTrue(false);

	return keyframe;
}

void anim::SampleChannel(const ast::AnimChannel &ch, float ts, JointPose *local_trs)
{
	InterpolatedKeyframe keyframe = InterpolateKeyframe(ch, ts);

	const ast::AnimKey *k0 = &ch.keys[keyframe.k0];
	const ast::AnimKey *k1 = &ch.keys[keyframe.k1];

	float progress = keyframe.progress;

	if (ch.interp == ast::AnimInterp_Step) {
		switch (ch.path) {
			case ast::AnimPath_Translate:
				local_trs->translation = k0->translation;
				break;

			case ast::AnimPath_Rotation:
				local_trs->rotation = k0->rotation;
				break;

			case ast::AnimPath_Scale:
				local_trs->scale = k0->scale;
				break;

			default:
				break;
		}
	} else if (ch.interp == ast::AnimInterp_Linear) {
		switch (ch.path) {
			case ast::AnimPath_Translate:
				local_trs->translation = k0->translation.Lerp(k1->translation, progress);
				break;

			case ast::AnimPath_Rotation:
				local_trs->rotation = k0->rotation.Slerp(k1->rotation, progress);
				break;

			case ast::AnimPath_Scale:
				local_trs->scale = k0->scale.Lerp(k1->scale, progress);
				break;

			default:
				break;
		}
	} else if (ch.interp == ast::AnimInterp_Cubic) {
		DebugPrintB("Cubic interpolation not yet implemented!");
	}
}

float anim::CalcSampleTime(float global_time, float global_start_time, float playback_rate, float duration, unsigned n)
{
	float sample_time = (global_time - global_start_time) * playback_rate;

	if (n > 0)
		sample_time = maths::Clamp(sample_time, 0.f, duration * static_cast<float>(n));

	if (duration <= 0.f)
		return 0.f;

	while (sample_time >= duration)
		sample_time -= duration;

	return sample_time;
}

anim::Animator::Animator()
	: playback_rate(1.f)
	, loop(true)
	, global_start_time(0.f)
	, paused(false)
	, global_paused_time(0.f)
	, last_sample_time(0.f)
{
}

anim::Animator::Animator(ast::Handle model_handle)
	: Animator()
{
	const ast::Asset *asset = ast::AssetSystem::Get()->GetOrFallback(model_handle);
	const ast::ModelAsset *asset_model = &asset->model;

	selected_model = model_handle;

	current_clip = ClipKey();

	poses.resize(asset_model->skeletons.size());

	for (int i = 0; i < poses.size(); i++) {
		const ast::Skeleton *skeleton = &asset_model->skeletons[i];

		u64 count = skeleton->joints.size();

		poses[i].local_poses.resize(count);
		poses[i].global_transforms.resize(count);
		poses[i].palette.resize(count);
	}
}

const ast::ModelAsset *anim::Animator::GetModelAsset() const
{
	return &ast::AssetSystem::Get()->GetOrFallback(selected_model)->model;
}

void anim::Animator::Tick(float global_time)
{
	if (poses.empty())
		return;

	const ast::ModelAsset *asset_model = GetModelAsset();

	// reset bind pose
	for (int i = 0; i < poses.size(); i++) {
		const ast::Skeleton *skeleton = &asset_model->skeletons[i];

		for (int j = 0; j < skeleton->joints.size(); j++) {
			poses[i].local_poses[j].translation = skeleton->joints[j].bind_translation;
			poses[i].local_poses[j].rotation = skeleton->joints[j].bind_rotation;
			poses[i].local_poses[j].scale = skeleton->joints[j].bind_scale;
		}
	}

	// advance and sample the clip
	if (!current_clip.IsNull() && current_clip.value < static_cast<int>(asset_model->clips.size())) {
		const ast::AnimClip *anim_clip = &asset_model->clips[current_clip.value];

		if (paused)
			global_time = global_paused_time;

		float sample_time = CalcSampleTime(global_time, global_start_time, playback_rate, anim_clip->duration_s, 0);

		for (const ast::AnimChannel &ch : anim_clip->channels) {
			if (ch.target_skeleton < 0 || ch.target_skeleton >= static_cast<int>(poses.size()))
				continue;

			SkeletonPose *pose = &poses[ch.target_skeleton];

			if (ch.target_joint >= pose->GetJointCount())
				continue;

			SampleChannel(ch, sample_time, &pose->local_poses[ch.target_joint]);
		}

		last_sample_time = sample_time;
	}
}

void anim::Animator::UpdatePalette()
{
	if (poses.empty())
		return;

	const ast::ModelAsset *asset_model = GetModelAsset();

	for (int i = 0; i < poses.size(); i++) {
		const ast::Skeleton *skeleton = &asset_model->skeletons[i];
		SkeletonPose *pose = &poses[i];

		for (int j = 0; j < skeleton->joints.size(); j++) {
			Mat4 local_transform = pose->local_poses[j].AsMatrix();

			int parent = skeleton->joints[j].parent;

			pose->global_transforms[j] = parent < 0
				? skeleton->root_parent_world * local_transform
				: pose->global_transforms[parent] * local_transform;

			pose->palette[j] = pose->global_transforms[j] * skeleton->joints[j].inverse_bind_matrix;
		}
	}
}

anim::ClipKey anim::Animator::FindClipByName(const String &name) const
{
	const ast::ModelAsset *asset_model = GetModelAsset();

	for (int i = 0; i < asset_model->clips.size(); i++) {
		if (asset_model->clips[i].name == name) {
			ClipKey key;
			key.value = i;
			return key;
		}
	}

	return ClipKey();
}

void anim::Animator::Play(ClipKey new_clip, bool new_loop, float new_global_start_time)
{
	if (new_clip.IsNull())
		return;

	paused = false;

	if (current_clip.value == new_clip.value && loop == new_loop)
		return;

	current_clip = new_clip;
	loop = new_loop;
	global_start_time = new_global_start_time;
}

void anim::Animator::Stop()
{
	current_clip = ClipKey();
	paused = true;
}

void anim::Animator::Resume()
{
	paused = false;
}

void anim::Animator::Pause(float global_time)
{
	if (paused)
		return;

	paused = true;
	global_paused_time = global_time;
}

void anim::Animator::PauseAndReset(float global_time)
{
	if (paused)
		return;

	paused = true;
	global_start_time = global_time;
	global_paused_time = global_time;
}

bool anim::Animator::IsFinished() const
{
	if (current_clip.IsNull() || loop)
		return false;

	const ast::ModelAsset *asset_model = GetModelAsset();
	const ast::AnimClip *anim_clip = &asset_model->clips[current_clip.value];

	return last_sample_time >= anim_clip->duration_s;
}

float anim::Animator::CalcNormalizedTimeForCurrentClip() const
{
	const ast::ModelAsset *asset_model = GetModelAsset();
	const ast::AnimClip *c = &asset_model->clips[current_clip.value];

	return last_sample_time / c->duration_s;
}

anim::Palette anim::Animator::GetPalette(int skin_index) const
{
	Palette palette;

	if (skin_index < 0 || skin_index >= poses.size())
		return palette;

	palette.matrices = poses[skin_index].palette.data();
	palette.joint_count = poses[skin_index].GetJointCount();

	return palette;
}

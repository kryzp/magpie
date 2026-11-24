#ifndef ANIMATION_ANIMATOR_H
#define ANIMATION_ANIMATOR_H

#include "core/mat4.h"
#include "core/quat.h"
#include "core/vec3.h"
#include "asset/handle.h"
#include "asset/types.h"

#include "container/string.h"
#include "container/vector.h"

namespace anim {

struct Palette {
	const Mat4 *matrices = nullptr;
	unsigned joint_count = 0;
};

struct JointPose {
	Vec3 translation;
	Quat rotation;
	Vec3 scale = Vec3(1.f);

	[[nodiscard]] Mat4 AsMatrix() const;
	[[nodiscard]] JointPose Blend(const JointPose &other, float u) const;
};

struct InterpolatedKeyframe {
	unsigned k0 = 0;
	unsigned k1 = 0;
	float progress = 0.f;
};

struct SkeletonPose {
	Vector<JointPose> local_poses;
	Vector<Mat4> global_transforms; // hierarchy accumulated
	Vector<Mat4> palette;

	[[nodiscard]] unsigned GetJointCount() const
	{
		return local_poses.size();
	}
};

struct ClipKey {
	int value = -1;

	[[nodiscard]] bool IsNull() const
	{
		return value < 0;
	}
};

float TimestampProgressFactor(float prev_ts, float next_ts, float ts);
InterpolatedKeyframe InterpolateKeyframe(const ast::AnimChannel &ch, float ts);
void SampleChannel(const ast::AnimChannel &ch, float ts, JointPose *local_trs);
float CalcSampleTime(float global_time, float global_start_time, float playback_rate, float duration, unsigned n);

class Animator {
public:
	Animator();
	explicit Animator(ast::Handle model_handle);

	void Tick(float global_time);
	void UpdatePalette();

	[[nodiscard]] ClipKey FindClipByName(const String &name) const;

	void Play(ClipKey clip, bool loop, float global_start_time);
	void Stop();
	void Resume();
	void Pause(float global_time);
	void PauseAndReset(float global_time);

	[[nodiscard]] bool IsFinished() const;
	[[nodiscard]] float CalcNormalizedTimeForCurrentClip() const;

	[[nodiscard]] Palette GetPalette(int skin_index) const;

	[[nodiscard]] ast::Handle GetModel() const
	{
		return selected_model;
	}

	float playback_rate;

private:
	[[nodiscard]] const ast::ModelAsset *GetModelAsset() const;

	ast::Handle selected_model;
	ClipKey current_clip;

	bool loop;
	float global_start_time;

	bool paused;
	float global_paused_time;

	Vector<SkeletonPose> poses;

	float last_sample_time;
};

}

#endif // ANIMATION_ANIMATOR_H

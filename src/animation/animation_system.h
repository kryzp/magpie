#ifndef ANIMATION_ANIMATION_SYSTEM_H
#define ANIMATION_ANIMATION_SYSTEM_H

#include "core/types.h"
#include "core/log.h"
#include "animation/animator.h"

#define ANIMATION_MAX_INSTANCES 128

namespace anim {

struct Handle {
	unsigned index = static_cast<unsigned>(-1);
	unsigned generation = 0;

	bool IsNull() const
	{
		return index == static_cast<unsigned>(-1) && generation == 0;
	}

	bool operator==(const Handle &other) const
	{
		return index == other.index && generation == other.generation;
	}

	bool operator!=(const Handle &other) const
	{
		return !(*this == other);
	}
};

struct Instance {
	Animator animator;

	bool alive = false;
	unsigned generation = 0;
};

// todo: make the instance pool dynamically allocated
class AnimationSystem {
public:
	explicit AnimationSystem(LogChannel log_channel);
	~AnimationSystem();

	AnimationSystem(const AnimationSystem &) = delete;
	AnimationSystem &operator=(const AnimationSystem &) = delete;

	static AnimationSystem *Get();

	void CalculateIntermediatePoses(float elapsed);
	void FinalizePoseAndMatrixPalette();

	Instance *Resolve(Handle handle);
	Animator *GetAnimator(Handle handle);

	void Play(Handle handle, ClipKey clip, bool loop, float global_start_time);
	bool IsFinished(Handle handle);
	Palette GetPalette(Handle handle, int skin_index);

	Handle CreateInstance(ast::Handle model_handle);
	void KillInstance(Handle handle);

private:
	LogChannel log_channel;

	Instance instances[ANIMATION_MAX_INSTANCES];
	unsigned instance_count;

	unsigned free_indices[ANIMATION_MAX_INSTANCES];
	unsigned free_index_count;
};
}

#endif // ANIMATION_ANIMATION_SYSTEM_H

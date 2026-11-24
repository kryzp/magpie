#include "animation/animation_system.h"

static anim::AnimationSystem *selected_system = nullptr;

anim::AnimationSystem *anim::AnimationSystem::Get()
{
	return selected_system;
}

anim::AnimationSystem::AnimationSystem(LogChannel log_channel_)
	: log_channel(log_channel_)
	, instance_count(0)
	, free_index_count(0)
{
	selected_system = this;

	DebugLogI(log_channel, "Initialized.");
}

anim::AnimationSystem::~AnimationSystem()
{
	DebugLogI(log_channel, "Destroyed.");

	if (selected_system == this)
		selected_system = nullptr;
}

void anim::AnimationSystem::CalculateIntermediatePoses(float elapsed)
{
	for (int i = 0; i < instance_count; i++) {
		Instance *inst = &instances[i];

		if (!inst->alive)
			continue;

		inst->animator.Tick(elapsed);
	}
}

void anim::AnimationSystem::FinalizePoseAndMatrixPalette()
{
	for (int i = 0; i < instance_count; i++) {
		Instance *inst = &instances[i];

		if (!inst->alive)
			continue;

		inst->animator.UpdatePalette();
	}
}

anim::Instance *anim::AnimationSystem::Resolve(Handle handle)
{
	if (handle.index >= ArraySize(instances))
		return nullptr;

	Instance *inst = &instances[handle.index];

	if (!inst->alive || inst->generation != handle.generation)
		return nullptr;

	return inst;
}

anim::Animator *anim::AnimationSystem::GetAnimator(Handle handle)
{
	Instance *inst = Resolve(handle);

	DebugLogAssert(log_channel, inst, "Handle is null.");

	return inst ? &inst->animator : nullptr;
}

void anim::AnimationSystem::Play(Handle handle, ClipKey clip, bool loop, float global_start_time)
{
	Animator *anim = GetAnimator(handle);

	if (!anim)
		return;

	anim->Play(clip, loop, global_start_time);
}

bool anim::AnimationSystem::IsFinished(Handle handle)
{
	Animator *anim = GetAnimator(handle);
	return anim ? anim->IsFinished() : false;
}

anim::Palette anim::AnimationSystem::GetPalette(Handle handle, int skin_index)
{
	Instance *inst = Resolve(handle);

	if (!inst) {
		DebugLogW(log_channel, "Handle is invalid, returning empty palette.");
		return Palette();
	}

	return inst->animator.GetPalette(skin_index);
}

anim::Handle anim::AnimationSystem::CreateInstance(ast::Handle model_handle)
{
	Handle result;

	unsigned index = 0;

	if (free_index_count > 0) {
		free_index_count--;
		index = free_indices[free_index_count];
	} else {
		if (instance_count >= ArraySize(instances)) {
			DebugLogE(
				log_channel,
				"Instance pool exhausted (max %lu).",
				ArraySize(instances)
			);

			return result;
		}

		index = instance_count;
		instance_count++;
	}

	Instance *inst = &instances[index];

	inst->animator = Animator(model_handle);
	inst->alive = true;

	result.index = index;
	result.generation = inst->generation;

	return result;
}

void anim::AnimationSystem::KillInstance(Handle handle)
{
	Instance *inst = Resolve(handle);

	if (!inst)
		return;

	inst->alive = false;
	inst->generation++;

	free_indices[free_index_count] = handle.index;
	free_index_count++;
}

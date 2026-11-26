#include "render/sprite_animator.h"

#include <algorithm>
#include <cmath>

rnd::SpriteAnimation::SpriteAnimation()
{
}

rnd::SpriteAnimation::SpriteAnimation(float frame_duration_, const Vector<rnd::TextureRegion> &frames_)
	: interval(frame_duration_)
	, frames(frames_)
{
}

float rnd::SpriteAnimation::Duration() const
{
	return interval * frames.size();
}

const rnd::TextureRegion *rnd::SpriteAnimation::GetKeyFrame(float time, bool looping) const
{
	if (frames.empty())
		return nullptr;

	int index = static_cast<int>(time / interval);

	if (looping)
		index = index % frames.size();
	else
		index = std::min(static_cast<int>(frames.size() - 1), index);

	index = std::max(0, index);

	return &frames[index];
}

rnd::SpriteAnimator::SpriteAnimator()
	: current(nullptr)
	, loops(0)
	, elapsed(0.f)
	, rate(1.f)
{
}

rnd::SpriteAnimator &rnd::SpriteAnimator::Define(const String &n, const SpriteAnimation &animation)
{
	animations[n] = animation;

	if (!current) {
		Play(n, 0);
	} else {
		// the map may have moved the playing animation's storage around
		current = &animations[name];
	}

	return *this;
}

rnd::SpriteAnimator &rnd::SpriteAnimator::SetStatic(const rnd::TextureRegion &region)
{
	return Define("static", SpriteAnimation(1.f, { region }));
}

void rnd::SpriteAnimator::Play(const String &n, int loops_)
{
	if (n == name && loops_ == loops && current)
		return;

	Restart(n, loops_);
}

void rnd::SpriteAnimator::PlayOnce(const String &n)
{
	Play(n, 1);
}

void rnd::SpriteAnimator::Restart(const String &n, int loops_)
{
	auto it = animations.find(n);

	if (it == animations.end())
		return;

	current = &it->second;
	name = n;
	loops = loops_;
	elapsed = 0.f;
}

void rnd::SpriteAnimator::Update(float dt)
{
	elapsed += dt * rate;
}

bool rnd::SpriteAnimator::Finished() const
{
	return current && loops > 0 && elapsed >= current->Duration() * loops;
}

float rnd::SpriteAnimator::NormalizedTime() const
{
	if (!current)
		return 0.f;

	float d = current->Duration();

	if (loops > 0 && elapsed >= d * loops)
		return 1.f;

	return std::fmod(elapsed, d) / d;
}

const rnd::TextureRegion *rnd::SpriteAnimator::GetFrame() const
{
	if (!current)
		return nullptr;

	float d = current->Duration();
	float t = std::fmod(elapsed, d);

	if (loops > 0 && elapsed >= d * loops)
		t = d;

	return current->GetKeyFrame(t, false);
}

#ifndef RENDER_SPRITE_ANIMATOR_H
#define RENDER_SPRITE_ANIMATOR_H

#include "render/canvas.h"

#include "container/hash_map.h"
#include "container/string.h"
#include "container/vector.h"

namespace rnd {

struct SpriteAnimation {
	float interval = 1.f;
	Vector<TextureRegion> frames;

	SpriteAnimation();
	SpriteAnimation(float frame_duration, const Vector<TextureRegion> &frames);

	[[nodiscard]] float Duration() const;
	[[nodiscard]] const TextureRegion *GetKeyFrame(float time, bool looping) const;
};

class SpriteAnimator {
public:
	SpriteAnimator();

	SpriteAnimator &Define(const String &name, const SpriteAnimation &animation);
	SpriteAnimator &SetStatic(const TextureRegion &region);

	void Play(const String &name, int loops);
	void PlayOnce(const String &name);
	void Restart(const String &name, int loops);

	void Update(float dt);

	bool Finished() const;
	float NormalizedTime() const;

	const TextureRegion *GetFrame() const;

private:
	HashMap<String, SpriteAnimation> animations;
	const SpriteAnimation *current;
	String name;
	int loops; // 0 = forever
	float elapsed;
	float rate;
};

}

#endif // RENDER_SPRITE_ANIMATOR_H

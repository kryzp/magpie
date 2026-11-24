#ifndef UI_ANIMATION_H
#define UI_ANIMATION_H

#include <functional>

#include "container/hash_map.h"
#include "container/string.h"

namespace ui {

struct AnimationState {
	float x = 0.f;
	float y = 0.f;
	float width = 0.f;
	float height = 0.f;
	float rotation = 0.f;
	float scale = 1.f;
	float alpha = 1.f;
};

class Transition {
public:
	typedef std::function<float(float)> Driver;

	Transition();

	void Apply(float t, AnimationState *out) const;

	Transition &XDriver(const Driver &driver);
	Transition &YDriver(const Driver &driver);
	Transition &WidthDriver(const Driver &driver);
	Transition &HeightDriver(const Driver &driver);
	Transition &RotationDriver(const Driver &driver);
	Transition &ScaleDriver(const Driver &driver);
	Transition &AlphaDriver(const Driver &driver);

private:
	Driver x;
	Driver y;
	Driver width;
	Driver height;
	Driver rotation;
	Driver scale;
	Driver alpha;
};

class Animator {
public:
	Animator();

	void AddChannel(const String &name, float rate, const Transition &transition);
	void SetTarget(const String &name, bool active);
	void Update(float dt);

	AnimationState &GetAnimationState();
	const AnimationState &GetAnimationState() const;

private:
	struct Channel {
		Transition transition;
		float rate = 0.f;
		float progress = 0.f;
		float target = 0.f;
	};

	HashMap<String, Channel> channels;
	AnimationState state;
};

}

#endif // UI_ANIMATION_H

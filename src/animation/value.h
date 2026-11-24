#ifndef ANIMATION_VALUE_H
#define ANIMATION_VALUE_H

#include "core/types.h"

namespace anim {

struct Value {
	u64 key = 0;
	float curr = 0.f;
	float target = 0.f;
};

struct ValueRegistry {
	Value values[512];
	unsigned value_count = 0;

	//void Tick(float dt);
};

}

#endif // ANIMATION_VALUE_H

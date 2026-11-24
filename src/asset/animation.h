#ifndef ASSET_ANIMATION_H
#define ASSET_ANIMATION_H

#include "core/maths.h"
#include "container/string.h"
#include "container/vector.h"

namespace ast {

enum AnimPath {
	AnimPath_Translate,
	AnimPath_Rotation,
	AnimPath_Scale,
	AnimPath_MaxEnum
};

enum AnimInterp {
	AnimInterp_Step,
	AnimInterp_Linear,
	AnimInterp_Cubic,
	AnimInterp_MaxEnum
};

struct AnimKey {
	float timestamp_s = 0.f;
	// only a specific one is relevant at a time
	// todo: yet another time i can't use a union because of ctors
	Vec3 translation;
	Quat rotation;
	Vec3 scale;
};

struct AnimChannel {
	int target_skeleton = -1; // -1 for no skeleton
	unsigned target_joint = 0;
	AnimPath path = AnimPath_Translate;
	AnimInterp interp = AnimInterp_Linear;
	Vector<AnimKey> keys;
};

struct AnimClip {
	String name;
	float duration_s = 0.f;
	Vector<AnimChannel> channels;
};

}

#endif // ASSET_ANIMATION_H

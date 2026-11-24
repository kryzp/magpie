#ifndef AUDIO_LISTENER_H
#define AUDIO_LISTENER_H

#include "core/vec3.h"

namespace aud {

struct Listener {
	Vec3 position;
	Vec3 direction;
};

}

#endif // AUDIO_LISTENER_H

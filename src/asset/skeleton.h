#ifndef ASSET_SKELETON_H
#define ASSET_SKELETON_H

#include "core/vec3.h"
#include "core/quat.h"
#include "core/mat4.h"
#include "container/string.h"
#include "container/vector.h"

namespace ast {

struct Joint {
	String name;
	int parent = -1;

	Vec3 bind_translation;
	Quat bind_rotation;
	Vec3 bind_scale = Vec3(1.f);

	Mat4 inverse_bind_matrix = Mat4::Identity();
};

struct Skeleton {
	String name;

	Vector<Joint> joints;

	// Root world transformation matrix applied to all joint transforms.
	// Sets the orientation.
	Mat4 root_parent_world = Mat4::Identity();
};

}

#endif // ASSET_SKELETON_H

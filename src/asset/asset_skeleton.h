#ifndef ASSET_SKELETON_H
#define ASSET_SKELETON_H

typedef struct A_Joint A_Joint;
struct A_Joint
{
	String8 name;

	i32 parent;

	v3 bind_translation;
	v4 bind_rotation;
	v3 bind_scale;
	
	m4 inverse_bind_matrix;
};

typedef struct A_Skeleton A_Skeleton;
struct A_Skeleton
{
	String8 name;
	
	u32 joint_count;
	A_Joint *joints;

	// Root world transformation matrix applied to all joint transforms.
	// Sets the orientation.
	m4 root_parent_world;
};

#endif // ASSET_SKELETON_H

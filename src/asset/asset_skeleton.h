#ifndef ASSET_SKELETON_H
#define ASSET_SKELETON_H

typedef struct AST_Joint AST_Joint;
struct AST_Joint
{
	String8 name;

	i32 parent;

	v3 bind_translate;
	v4 bind_rotation;
	v3 bind_scale;
	
	m4 inverse_bind_matrix;
};

typedef struct AST_Skeleton AST_Skeleton;
struct AST_Skeleton
{
	String8 name;
	
	u32 joint_count;
	AST_Joint *joints;

	m4 root_parent_world;
};

#endif // ASSET_SKELETON_H

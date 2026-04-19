#ifndef ANIMATION_BONE
#define ANIMATION_BONE

typedef struct ANIM_KeyPosition ANIM_KeyPosition;
struct ANIM_KeyPosition
{
	f32 timestamp;
	v3 position;
};

typedef struct ANIM_KeyRotation ANIM_KeyRotation;
struct ANIM_KeyRotation
{
	f32 timestamp;
	v4 rotation;
};

typedef struct ANIM_KeyScale ANIM_KeyScale;
struct ANIM_KeyScale
{
	f32 timestamp;
	v3 scale;
};

typedef struct ANIM_Bone ANIM_Bone;
struct ANIM_Bone
{
	b32 temp;
};

#endif // ANIMATION_BONE

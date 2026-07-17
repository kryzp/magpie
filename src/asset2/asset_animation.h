#ifndef ASSET_ANIMATION_H
#define ASSET_ANIMATION_H

typedef enum A_AnimPath
{
	A_AnimPath_Translate,
	A_AnimPath_Rotation,
	A_AnimPath_Scale,
	A_AnimPath_COUNT
}
A_AnimPath;

typedef enum A_AnimInterp
{
	A_AnimInterp_Step,
	A_AnimInterp_Linear,
	A_AnimInterp_Cubic,
	A_AnimInterp_COUNT
}
A_AnimInterp;

typedef struct A_AnimKey A_AnimKey;
struct A_AnimKey
{
	f32 timestamp_s;

	union
	{
		v3 translation;
		v4 rotation;
		v3 scale;
	};
};

typedef struct A_AnimChannel A_AnimChannel;
struct A_AnimChannel
{
	i32 target_skeleton; // -1 for no skeleton
	u32 target_joint;

	A_AnimPath path;
	A_AnimInterp interp;
	
	u32 key_count;
	A_AnimKey *keys;
};

typedef struct A_AnimClip A_AnimClip;
struct A_AnimClip
{
	String8 name;
	f32 duration_s;
	
	u32 channel_count;
	A_AnimChannel *channels;
};

#endif // ASSET_ANIMATION_H

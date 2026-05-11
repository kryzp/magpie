#ifndef ASSET_ANIMATION_H
#define ASSET_ANIMATION_H

typedef enum AST_AnimPath
{
	AST_AnimPath_Translate,
	AST_AnimPath_Rotation,
	AST_AnimPath_Scale,
	AST_AnimPath_COUNT
}
AST_AnimPath;

typedef enum AST_AnimInterp
{
	AST_AnimInterp_Step,
	AST_AnimInterp_Linear,
	AST_AnimInterp_Cubic,
	AST_AnimInterp_COUNT
}
AST_AnimInterp;

typedef struct AST_AnimKey AST_AnimKey;
struct AST_AnimKey
{
	f32 timestamp_s;

	union
	{
		v3 translation;
		v4 rotation;
		v3 scale;
	};
};

typedef struct AST_AnimChannel AST_AnimChannel;
struct AST_AnimChannel
{
	i32 target_skeleton; // -1 for no skeleton
	u32 target_joint;

	AST_AnimPath path;
	AST_AnimInterp interp;
	
	u32 key_count;
	AST_AnimKey *keys;
};

typedef struct AST_AnimClip AST_AnimClip;
struct AST_AnimClip
{
	String8 name;

	f32 duration_s;
	
	u32 channel_count;
	AST_AnimChannel *channels;
};

#endif // ASSET_ANIMATION_H

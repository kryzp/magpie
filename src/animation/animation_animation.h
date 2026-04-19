#ifndef ANIMATION_ANIMATION_H
#define ANIMATION_ANIMATION_H

typedef struct ANIM_Node ANIM_Node;
struct ANIM_Node
{
	ANIM_Node *sibling_next;
	String8 name;
	m4 transform;
	ANIM_Node *children;
};

typedef struct ANIM_Animation ANIM_Animation;
struct ANIM_Animation
{
	b32 temp;
};

#endif // ANIMATION_ANIMATION_H

#ifndef PHYSICS_HANDLE_H
#define PHYSICS_HANDLE_H

typedef struct P_Handle P_Handle;
struct P_Handle
{
	u64 key;
};

static inline b32 P_HandleMatch(P_Handle a, P_Handle b)
{
	return a.key == b.key;
}

#endif // PHYSICS_HANDLE_H

#ifndef ENTITY_HEADER_H
#define ENTITY_HEADER_H

typedef struct E_Handle E_Handle;
struct E_Handle
{
	u32 tid;
	u32 slot;
	u32 generation;
};

static inline E_Handle E_HandleNull(void)
{
	E_Handle null_handle = {0};
	return null_handle;
}

static inline b32 E_HandleIsNull(E_Handle handle)
{
	return handle.generation == 0;
}

static inline b32 E_HandleMatch(E_Handle a, E_Handle b)
{
	return (
		a.tid == b.tid &&
		a.slot == b.slot &&
		a.generation == b.generation
	);
}

typedef u32 E_Flags;
enum
{
	E_Flag_None        = 0,
	E_Flag_Active      = 1 << 0, // is ticked
	E_Flag_PendingKill = 1 << 1  // marked for death (Aura)
};

/*
 * Bascially to do basically what C++ does
 * internally to mimick inheritance.
 * you gotta dump this struct onto
 * the top of each entity. It's got the most core
 * data required for entities, and means you can
 * cast any entity to (E_Header *) to do generic
 * operations.
 */
typedef struct E_Header E_Header;
struct E_Header
{
	E_Handle handle;
	E_Flags flags;
	E_Transform transform;
	u16 layer_id;
	//E_Handle parent; TODO
	//b32 inherit_transform; TODO
};

#define E_HeaderOf(entity_pointer) ((E_Header *)(entity_pointer))
#define E_TransformOf(entity_pointer) (&(E_HeaderOf(entity_pointer)->transform))

#endif // ENTITY_HEADER_H

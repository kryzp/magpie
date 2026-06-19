#ifndef ENTITY_HEADER_H
#define ENTITY_HEADER_H

typedef struct E_UID E_UID;
struct E_UID
{
	u32 value;
};

internal inline E_UID
E_UIDNull(void)
{
	return (E_UID) {0};
}

internal inline b32
E_UIDIsNull(E_UID uid)
{
	return uid.value == 0;
}

internal inline b32
E_UIDMatch(E_UID a, E_UID b)
{
	return a.value == b.value;
}

typedef u32 E_Flags;
enum
{
	E_Flag_None        = 0,
	E_Flag_Active      = 1 << 0, // is ticked
	E_Flag_Visible     = 1 << 1, // is rendered
	E_Flag_Static      = 1 << 2, // doesn't move (more of a hint typically)
	E_Flag_PendingKill = 1 << 3  // marked for death (Aura)
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
	E_UID uid;
	E_Type type;
	E_Flags flags;
	u16 layer_id;
	E_Transform transform;
	String8 debug_name;
};

#define E_HeaderOf(entity_pointer) ((E_Header *)(entity_pointer))

#define E_UIDOf(entity_pointer)           (E_HeaderOf(entity_pointer)->uid)
#define E_TypeOf(entity_pointer)          (E_HeaderOf(entity_pointer)->type)
#define E_FlagsOf(entity_pointer)         (E_HeaderOf(entity_pointer)->flags)
#define E_LayerIDOf(entity_pointer)       (E_HeaderOf(entity_pointer)->layer_id)
#define E_TransformOf(entity_pointer)     (E_HeaderOf(entity_pointer)->transform)
#define E_DebugNameOf(entity_pointer)     (E_HeaderOf(entity_pointer)->debug_name)

#endif // ENTITY_HEADER_H

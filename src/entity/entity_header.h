#ifndef ENTITY_HEADER_H
#define ENTITY_HEADER_H

typedef struct ENT_UID ENT_UID;
struct ENT_UID
{
	u32 value;
};

internal inline ENT_UID
ENT_UIDNull(void)
{
	return (ENT_UID) {0};
}

internal inline b32
ENT_UIDIsNull(ENT_UID uid)
{
	return uid.value == 0;
}

internal inline b32
ENT_UIDMatch(ENT_UID a, ENT_UID b)
{
	return a.value == b.value;
}

typedef u32 ENT_Flags;
enum
{
	ENT_Flag_None        = 0,
	ENT_Flag_Active      = 1 << 0, // is ticked
	ENT_Flag_Visible     = 1 << 1, // is rendered
	ENT_Flag_Static      = 1 << 2, // doesn't move (more of a hint typically)
	ENT_Flag_PendingKill = 1 << 3  // marked for death (Aura)
};

/*
 * Bascially to do basically what C++ does
 * internally to mimick inheritance.
 * you gotta dump this struct onto
 * the top of each entity. It's got the most core
 * data required for entities, and means you can
 * cast any entity to (ENT_Header *) to do generic
 * operations.
 */
typedef struct ENT_Header ENT_Header;
struct ENT_Header
{
	ENT_UID uid;
	ENT_Type type;
	ENT_Flags flags;
	u16 layer_id;
	ENT_Transform transform;
	String8 debug_name;
};

#define ENT_HeaderOf(entity_pointer) ((ENT_Header *)(entity_pointer))

// todo: not a fan of this, should probably just
//       remove and use header directly.
#define ENT_UIDOf(entity_pointer)           (ENT_HeaderOf(entity_pointer)->uid)
#define ENT_TypeOf(entity_pointer)          (ENT_HeaderOf(entity_pointer)->type)
#define ENT_FlagsOf(entity_pointer)         (ENT_HeaderOf(entity_pointer)->flags)
#define ENT_LayerIDOf(entity_pointer)       (ENT_HeaderOf(entity_pointer)->layer_id)
#define ENT_TransformOf(entity_pointer)     (ENT_HeaderOf(entity_pointer)->transform)
#define ENT_DebugNameOf(entity_pointer)     (ENT_HeaderOf(entity_pointer)->debug_name)

#endif // ENTITY_HEADER_H

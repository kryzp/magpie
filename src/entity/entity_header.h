#ifndef ENTITY_HEADER_H
#define ENTITY_HEADER_H

typedef struct E_UID E_UID;
struct E_UID
{
	u32 value;
};

static inline b32 E_UIDMatch(E_UID a, E_UID b)
{
	return a.value == b.value;
}

typedef struct E_TID
{
	u32 value;
}
E_TID;

static inline b32 E_TIDMatch(E_TID a, E_TID b)
{
	return a.value == b.value;
}

typedef struct E_Handle E_Handle;
struct E_Handle
{
	E_UID uid;
	E_TID tid;
	u32 generation;
};

static inline b32 E_HandleMatch(E_Handle a, E_Handle b)
{
	return true;
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
	E_UID uid;
	E_TID tid;
	E_Flags flags;
	u16 layer_id;
};

#define E_HeaderOf(entity_pointer) ((E_Header *)(entity_pointer))

#endif // ENTITY_HEADER_H

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

// Type of the entity - Player, ShopKeepr, etc...
typedef struct ENT_TypeID ENT_TypeID;
struct ENT_TypeID
{
	u16 value;
};

typedef u32 ENT_Flags;
enum
{
	ENT_Flag_None        = 0,
	ENT_Flag_Active      = 1 << 0, // is ticked
	ENT_Flag_Visible     = 1 << 1, // is rendered
	ENT_Flag_Static      = 1 << 2, // doesn't move (more of a hint typically)
	ENT_Flag_PendingKill = 1 << 3  // marked for death (Aura)
};

typedef struct ENT_Transform ENT_Transform;
struct ENT_Transform
{
	v3 position;
	v4 rotation;
	v3 scale;
	m4 matrix;
	b32 dirty;
};

internal void ENT_TransformRecompute(ENT_Transform *transform);

// I hate getters / setters but we need these to automatically
// set the dirty flag, though I wonder if we even need that...
internal void ENT_TransformSetPosition(ENT_Transform *transform, v3 position);
internal void ENT_TransformSetRotation(ENT_Transform *transform, v4 rotation);

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
	ENT_TypeID tid;
	ENT_Flags flags;
	u16 layer_id;
	ENT_Transform transform;
	String8 debug_name;
};

#endif // ENTITY_HEADER_H

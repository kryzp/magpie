#ifndef ENTITY_DUMMY_H
#define ENTITY_DUMMY_H

typedef struct ENT_EntityDummy ENT_EntityDummy;
struct ENT_EntityDummy
{
	ENT_Header header;
	// ---
	u32 misc_data;
	i16 etc;
	b32 random_flag;
};

internal void ENT_EntityDummyInit    (ENT_EntityDummy *dummy, v3 spawn_position);
internal void ENT_EntityDummyDestroy (ENT_EntityDummy *dummy);

internal void ENT_EntityDummyPreAnimTick     (ENT_EntityDummy *dummy, f32 dt);
internal void ENT_EntityDummyPostAnimTick    (ENT_EntityDummy *dummy, f32 dt);
internal void ENT_EntityDummyPostPhysicsTick (ENT_EntityDummy *dummy, f32 dt);

internal void ENT_EntityDummySerialize   (ENT_EntityDummy *dummy, IO_ByteWriter *writer);
internal void ENT_EntityDummyDeserialize (ENT_EntityDummy *dummy, IO_ByteReader *reader);

#endif // ENTITY_DUMMY_H

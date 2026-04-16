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

internal void ENT_EntityDummyInit    (void *entity, v3 spawn_position);
internal void ENT_EntityDummyDestroy (void *entity);

internal void ENT_EntityDummyPreAnimTick     (void *entity, f32 dt);
internal void ENT_EntityDummyPostAnimTick    (void *entity, f32 dt);
internal void ENT_EntityDummyPostPhysicsTick (void *entity, f32 dt);

internal void ENT_EntityDummySerialize   (void *entity, IO_ByteWriter *writer);
internal void ENT_EntityDummyDeserialize (void *entity, IO_ByteReader *reader);

#endif // ENTITY_DUMMY_H

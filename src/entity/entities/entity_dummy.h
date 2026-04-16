#ifndef ENTITY_DUMMY_H
#define ENTITY_DUMMY_H

typedef struct ENT_Dummy ENT_Dummy;
struct ENT_Dummy
{
	ENT_Header header;

	// ---
	
	u32 misc_data;
	i16 etc;
	b32 random_flag;
};

internal void ENT_DummyInit    (ENT_Dummy *dummy, v3 spawn_position);
internal void ENT_DummyDestroy (ENT_Dummy *dummy);

internal void ENT_DummyPreAnimTick     (ENT_Dummy *dummy, f32 dt);
internal void ENT_DummyPostAnimTick    (ENT_Dummy *dummy, f32 dt);
internal void ENT_DummyPostPhysicsTick (ENT_Dummy *dummy, f32 dt);

internal void ENT_DummySerialize   (ENT_Dummy *dummy, IO_ByteWriter *writer);
internal void ENT_DummyDeserialize (ENT_Dummy *dummy, IO_ByteReader *reader);

#endif // ENTITY_DUMMY_H

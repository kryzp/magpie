#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

// TODO: Implement!!
typedef struct IO_ByteWriter IO_ByteWriter;
typedef struct IO_ByteReader IO_ByteReader;

typedef struct ENT_World ENT_World;

typedef struct ENT_TypeDesc ENT_TypeDesc;
struct ENT_TypeDesc
{
	String8 name;
	ENT_TypeID tid;
	u64 stride;
	u32 max_instances;

	void (*OnDestroy)         (void *entity);

	void (*OnPreAnimTick)     (void *entity, f32 dt);
	void (*OnPostAnimTick)    (void *entity, f32 dt);
	void (*OnPostPhysicsTick) (void *entity, f32 dt);
	
	void (*OnSerialize)       (void *entity, IO_ByteWriter *writer);
	void (*OnDeserialize)     (void *entity, IO_ByteReader *reader);
};

#endif // ENTITY_TYPE_H

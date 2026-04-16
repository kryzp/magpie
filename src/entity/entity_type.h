#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

// TODO: Implement!!
typedef struct IO_ByteWriter IO_ByteWriter;
typedef struct IO_ByteReader IO_ByteReader;

typedef struct ENT_World ENT_World;

typedef enum ENT_Type
{
	ENT_Type_Null = 0,
#define EntityDef(pascal, lower, max) ENT_Type_##Pascal,
#include "entity_xmacro.inc"
#undef EntityDef
	ENT_Type_COUNT,
}
ENT_Type;

typedef void ENT_TypeDescOnDestroyFn     (void *entity);
typedef void ENT_TypeDescOnTickFn        (void *entity, f32 dt);
typedef void ENT_TypeDescOnSerializeFn   (void *entity, IO_ByteWriter *writer);
typedef void ENT_TypeDescOnDeserializeFn (void *entity, IO_ByteReader *reader);

typedef struct ENT_TypeDesc ENT_TypeDesc;
struct ENT_TypeDesc
{
	String8 name;
	ENT_Type type;
	u64 stride;
	u32 max_instances;

	ENT_TypeDescOnDestroyFn *OnDestroy;
	
	ENT_TypeDescOnTickFn *OnPreAnimTick;
	ENT_TypeDescOnTickFn *OnPostAnimTick;
	ENT_TypeDescOnTickFn *OnPostPhysicsTick;

	ENT_TypeDescOnSerializeFn   *OnSerialize;
	ENT_TypeDescOnDeserializeFn *OnDeserialize;
};

#endif // ENTITY_TYPE_H

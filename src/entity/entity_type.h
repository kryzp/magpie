#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

// TODO: Implement!!
typedef struct IO_ByteWriter IO_ByteWriter;
typedef struct IO_ByteReader IO_ByteReader;

typedef enum ENT_Type
{
	ENT_Type_Null = 0,
#define EntityDef(pascal, lower, max) ENT_Type_##Pascal,
#include "entity_xmacro.inc"
#undef EntityDef
	ENT_Type_COUNT,
}
ENT_Type;

typedef struct ENT_World ENT_World;
typedef struct ENT_EventQueue ENT_EventQueue;

typedef struct ENT_TickContext ENT_TickContext;
struct ENT_TickContext
{
	ENT_World *world;
	ENT_EventQueue *events;
	
	f32 dt;

	const I_InputSt *input;
};

typedef void ENT_TypeDescDestroyFn     (void *entity);
typedef void ENT_TypeDescTickFn        (void *entity, const ENT_TickContext *ctx);
typedef void ENT_TypeDescSerializeFn   (void *entity, IO_ByteWriter *writer);
typedef void ENT_TypeDescDeserializeFn (void *entity, IO_ByteReader *reader);

typedef struct ENT_TypeDesc ENT_TypeDesc;
struct ENT_TypeDesc
{
	String8 name;
	ENT_Type type;
	u64 stride;
	u32 max_instances;

	ENT_TypeDescDestroyFn *OnDestroy;
	
	ENT_TypeDescTickFn *OnPreAnimTick;
	ENT_TypeDescTickFn *OnPostAnimTick;
	ENT_TypeDescTickFn *OnPostPhysicsTick;

	ENT_TypeDescSerializeFn   *OnSerialize;
	ENT_TypeDescDeserializeFn *OnDeserialize;
};

#endif // ENTITY_TYPE_H

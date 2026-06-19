#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

typedef enum E_Type
{
	E_Type_Null = 0,
#define EntityDef(pascal, lower, max) E_Type_##Pascal,
#include "entity_xmacro.inc"
#undef EntityDef
	E_Type_COUNT,
}
E_Type;

typedef struct E_World E_World;
typedef struct E_EventQueue E_EventQueue;

typedef struct E_TickContext E_TickContext;
struct E_TickContext
{
	E_World *world;
	E_EventQueue *events;
	
	const OS_InputState *input;
	
	f32 dt;
};

typedef void E_TypeDescDestroyFn     (void *entity);
typedef void E_TypeDescTickFn        (void *entity, const E_TickContext *ctx);
typedef void E_TypeDescSerializeFn   (void *entity, IO_ByteSerializer *writer);
typedef void E_TypeDescDeserializeFn (void *entity, IO_ByteSerializer *reader);

typedef struct E_TypeDesc E_TypeDesc;
struct E_TypeDesc
{
	String8 name;
	E_Type type;
	u64 stride;
	u32 max_instances;

	E_TypeDescDestroyFn *OnDestroy;
	
	E_TypeDescTickFn *OnPreAnimTick;
	E_TypeDescTickFn *OnPostAnimTick;
	E_TypeDescTickFn *OnPostPhysicsTick;

	E_TypeDescSerializeFn   *OnSerialize;
	E_TypeDescDeserializeFn *OnDeserialize;
};

#endif // ENTITY_TYPE_H

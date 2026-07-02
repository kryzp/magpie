#ifndef ENTITY_TYPE_H
#define ENTITY_TYPE_H

typedef struct E_World E_World;
typedef struct E_EventQueue E_EventQueue;

typedef struct E_TickContext E_TickContext;
struct E_TickContext
{
	f32 dt;
	f32 elapsed;
	const OS_InputState *input;
};

typedef void E_TypeDescInitFn        (void *entity, E_Transform transform);
typedef void E_TypeDescDestroyFn     (void *entity);
typedef void E_TypeDescTickFn        (void *entity, const E_TickContext *ctx);
typedef void E_TypeDescSerializeFn   (void *entity, IO_ByteSerializer *writer);
typedef void E_TypeDescDeserializeFn (void *entity, IO_ByteSerializer *reader);

typedef struct E_TypeDesc E_TypeDesc;
struct E_TypeDesc
{
	String8 name;
	u64 stride;
	u32 max_instances;

	E_TypeDescInitFn *OnInit;
	E_TypeDescDestroyFn *OnDestroy;
	
	E_TypeDescTickFn *OnPreAnimTick;
	E_TypeDescTickFn *OnPostAnimTick;
	E_TypeDescTickFn *OnPostPhysicsTick;

	E_TypeDescSerializeFn   *OnSerialize;
	E_TypeDescDeserializeFn *OnDeserialize;
};

#endif // ENTITY_TYPE_H

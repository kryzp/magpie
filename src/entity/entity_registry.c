
internal void
E_RegistryPopulate(E_World *world)
{
#if 0
#define EntityDef(pascal, lower, max) ent_global_struct_names[E_Type_##pascal] = String8Lit(STRINGIFY(pascal));
#include "entity_xmacro.inc"
#undef EntityDef
	
#define EntityDef(pascal, lower, max) ent_global_lower_names[E_Type_##pascal] = String8Lit(STRINGIFY(lower));
#include "entity_xmacro.inc"
#undef EntityDef
#endif
	
#define EntityDef(pascal, lower, max)									\
	{																	\
		E_TypeDesc desc = {												\
			.name              = String8Lit(STRINGIFY(pascal)),			\
			.type              = E_Type_##pascal,						\
			.stride            = sizeof(E_##pascal),					\
			.max_instances     = (max),									\
			.OnDestroy         = (E_TypeDescDestroyFn *)##pascal##Destroy, \
			.OnPreAnimTick     = (E_TypeDescTickFn *)##pascal##PreAnimTick, \
			.OnPostAnimTick    = (E_TypeDescTickFn *)##pascal##PostAnimTick, \
			.OnPostPhysicsTick = (E_TypeDescTickFn *)##pascal##PostPhysicsTick, \
			.OnSerialize       = (E_TypeDescSerializeFn *)##pascal##Serialize, \
			.OnDeserialize     = (E_TypeDescDeserializeFn *)##pascal##Deserialize \
		};																\
		E_WorldRegisterType(world, &desc);								\
	}

#include "entity_xmacro.inc"

#undef EntityDef
}


internal void
ED_RegistryPopulate(ENT_World *world)
{
#if 0
#define EntityDef(pascal, lower, max) ent_global_struct_names[ENT_Type_##pascal] = String8Lit(STRINGIFY(pascal));
#include "entity_xmacro.inc"
#undef EntityDef
	
#define EntityDef(pascal, lower, max) ent_global_lower_names[ENT_Type_##pascal] = String8Lit(STRINGIFY(lower));
#include "entity_xmacro.inc"
#undef EntityDef
#endif
	
#define EntityDef(pascal, lower, max)									\
	{																	\
		ENT_TypeDesc desc = {											\
			.name              = String8Lit(STRINGIFY(pascal)),			\
			.type              = ENT_Type_##pascal,						\
			.stride            = sizeof(ENT_##pascal),					\
			.max_instances     = (max),									\
			.OnDestroy         = (ENT_TypeDescDestroyFn *)##pascal##Destroy, \
			.OnPreAnimTick     = (ENT_TypeDescTickFn *)##pascal##PreAnimTick, \
			.OnPostAnimTick    = (ENT_TypeDescTickFn *)##pascal##PostAnimTick, \
			.OnPostPhysicsTick = (ENT_TypeDescTickFn *)##pascal##PostPhysicsTick, \
			.OnSerialize       = (ENT_TypeDescSerializeFn *)##pascal##Serialize, \
			.OnDeserialize     = (ENT_TypeDescDeserializeFn *)##pascal##Deserialize \
		};																\
		ENT_WorldRegisterType(world, &desc);							\
	}

#include "entity/entity_xmacro.inc"

#undef EntityDef
}

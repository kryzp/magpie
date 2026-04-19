
internal void
ENT_RegistryPopulate(void)
{
#define EntityDef(pascal, lower, max) ent_global_struct_names[ENT_Type_##pascal] = String8Lit(STRINGIFY(pascal));
#include "entity_xmacro.inc"
#undef EntityDef
	
#define EntityDef(pascal, lower, max) ent_global_lower_names[ENT_Type_##pascal] = String8Lit(STRINGIFY(lower));
#include "entity_xmacro.inc"
#undef EntityDef
	
#define EntityDef(pascal, lower, max)									\
	ent_global_types[ENT_Type_##Pascal] = {								\
		.name              = String8Lit(STRINGIFY(pascal)),				\
		.stride            = sizeof(ENT_##pascal),						\
		.max_instances     = (max),										\
		.OnDestroy         = (ENT_TypeDescDestroyFn *)ENT_##pascal##Destroy, \
		.OnPreAnimTick     = (ENT_TypeDescTickFn *)ENT_##pascal##PreAnimTick, \
		.OnPostAnimTick    = (ENT_TypeDescTickFn *)ENT_##pascal##PostAnimTick, \
		.OnPostPhysicsTick = (ENT_TypeDescTickFn *)ENT_##pascal##PostPhysicsTick, \
		.OnSerialize       = (ENT_TypeDescSerializeFn *)ENT_##pascal##Serialize, \
		.OnDeserialize     = (ENT_TypeDescDeserializeFn *)ENT_##pascal##Deserialize \
	};																	\

#include "entity_xmacro.inc"

#undef EntityDef
}

#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

global String8 ent_global_struct_names[ENT_Type_COUNT] =
{
#define EntityDef(pascal, lower, max) Str8(STRINGIFY(pascal)),
#include "entity_xmacro.inc"
#undef EntityDef
};

global String8 ent_global_lower_names[ENT_Type_COUNT] =
{
#define EntityDef(pascal, lower, max) Str8(STRINGIFY(lower)),
#include "entity_xmacro.inc"
#undef EntityDef
};

global ENT_TypeDesc ent_global_types[ENT_Type_COUNT] =
{
#define EntityDef(pascal, lower, max)									\
	[ENT_Type_##Pascal] =												\
	{																	\
		.name              = Str8(STRINGIFY(pascal)),					\
		.stride            = sizeof(ENT_##pascal),						\
		.max_instances     = (max),										\
		.OnDestroy         = (ENT_TypeDescDestroyFn *)ENT_##pascal##Destroy, \
		.OnPreAnimTick     = (ENT_TypeDescTickFn *)ENT_##pascal##PreAnimTick, \
		.OnPostAnimTick    = (ENT_TypeDescTickFn *)ENT_##pascal##PostAnimTick, \
		.OnPostPhysicsTick = (ENT_TypeDescTickFn *)ENT_##pascal##PostPhysicsTick, \
		.OnSerialize       = (ENT_TypeDescSerializeFn *)ENT_##pascal##Serialize, \
		.OnDeserialize     = (ENT_TypeDescDeserializeFn *)ENT_##pascal##Deserialize \
	},																	\

#include "entity_xmacro.inc"

#undef EntityDef
};

#endif // ENTITY_REGISTRY_H

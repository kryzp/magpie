
internal void
ENT_RegisterAllEntityTypes(ENT_Registry *registry)
{
#define EntityDef(name, lower, max_instances)							\
	do																	\
	{																	\
		ENT_TypeDesc type_desc = {0};									\
		type_desc.name = STRINGIFY(name);								\
		type_desc.stride = sizeof(name);								\
		type_desc.max_instances = max_instances;						\
		type_desc.OnDestroy         = ENT_Entity##name##Destroy;		\
		type_desc.OnPreAnimTick     = ENT_Entity##name##PreAnimTick;	\
		type_desc.OnPostAnimTick    = ENT_Entity##name##PostAnimTick;	\
		type_desc.OnPostPhysicsTick = ENT_Entity##name##PostPhysicsTick; \
		type_desc.OnSerialize       = ENT_Entity##name##Serialize;		\
		type_desc.OnDeserialize     = ENT_Entity##name##Deserialize;	\
		ent_type_##lower = ENT_RegistryAdd(registry, &type_desc);		\
	}																	\
	while (0);

#include "entity_xmacro.inc"

#undef EntityDef
}

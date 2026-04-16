
internal void
ENT_RegisterAllEntityTypes(ENT_Registry *registry)
{
#define EntityDef(pascal, lower, max_instance_count)					\
	do																	\
	{																	\
		ENT_TypeDesc type_desc = {0};									\
		type_desc.name              = Str8(STRINGIFY(pascal));			\
		type_desc.stride            = sizeof(ENT_Entity##pascal);		\
		type_desc.max_instances     = max_instance_count;				\
		type_desc.OnDestroy         = ENT_Entity##pascal##Destroy;		\
		type_desc.OnPreAnimTick     = ENT_Entity##pascal##PreAnimTick;	\
		type_desc.OnPostAnimTick    = ENT_Entity##pascal##PostAnimTick; \
		type_desc.OnPostPhysicsTick = ENT_Entity##pascal##PostPhysicsTick; \
		type_desc.OnSerialize       = ENT_Entity##pascal##Serialize;	\
		type_desc.OnDeserialize     = ENT_Entity##pascal##Deserialize;	\
																		\
		ent_type_##lower = ENT_RegistryAdd(registry, &type_desc);		\
	}																	\
	while (0);

#include "entity_xmacro.inc"

#undef EntityDef
}

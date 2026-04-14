#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

#define ENT_REGISTRY_MAX_TYPES 512

typedef struct ENT_Registry ENT_Registry;
struct ENT_Registry
{
	ENT_TypeDesc types[ENT_REGISTRY_MAX_TYPES];
	u32 count;
};

internal void       ENT_RegistryInit (ENT_Registry *registry);
internal ENT_TypeID ENT_RegistryAdd  (ENT_Registry *registry, const ENT_TypeDesc *desc);

internal const ENT_TypeDesc *ENT_RegistryGet       (const ENT_Registry *registry, ENT_TypeID tid);
internal const ENT_TypeDesc *ENT_RegistryGetByName (const ENT_Registry *registry, String8 name);

#endif // ENTITY_REGISTRY_H

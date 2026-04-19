#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

global String8 ent_global_struct_names[ENT_Type_COUNT];
global String8 ent_global_lower_names[ENT_Type_COUNT];
global ENT_TypeDesc ent_global_types[ENT_Type_COUNT];

internal void ENT_RegistryPopulate(void);

#endif // ENTITY_REGISTRY_H

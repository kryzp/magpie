#ifndef ENTITY_REGISTER_H
#define ENTITY_REGISTER_H

internal void ENT_RegisterAllEntityTypes(ENT_Registry *registry);

#define EntityDef(pascal, lower, max_instances) global ENT_TypeID ent_type_##lower;
#include "entity_xmacro.inc"
#undef EntityDef

#endif // ENTITY_REGISTER_H

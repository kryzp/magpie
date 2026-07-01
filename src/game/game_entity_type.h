#ifndef GAME_ENTITY_TYPE_H
#define GAME_ENTITY_TYPE_H

typedef enum GameEntityType
{
#define GameEntityDef(type, max) GameEntityType_##type,
#include "game_entity_xmacro.inc"
#undef GameEntityDef
    GameEntityType_COUNT
}
GameEntityType;

#endif // GAME_ENTITY_TYPE_H

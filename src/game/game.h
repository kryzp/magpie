#ifndef GAME_H
#define GAME_H

typedef struct Game Game;
struct Game
{
    u32 entity_types[GameEntityType_COUNT];
    E_Handle player_handle;
	GM_Stack game_mode_stack;
	R_Camera camera;
	CameraDriver camera_driver;
	b32 camera_driver_active;
};

static void GameRegisterEntities(Game *game, E_World *world);
static void GameInit(Game *game, E_World *world);
static void GameTick(Game *game, const OS_InputState *input, f32 dt, f32 elapsed);

#endif // GAME_H

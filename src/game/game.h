#ifndef GAME_H
#define GAME_H

typedef struct Game Game;
struct Game
{
	u32 entity_types[GameEntityType_COUNT];
	E_Handle player_handle;
	GameStateStack game_state_stack;
	R_Camera camera;
	CameraDriver camera_driver;
	b32 camera_driver_active;
};

internal void GameSelect(Game *game_);
internal void GameInit(E_World *world);
internal void GameTick(const OS_InputState *input, f32 dt, f32 elapsed);

static Game *game = NULL;

#endif // GAME_H

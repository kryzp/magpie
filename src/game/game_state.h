#ifndef GAME_STATE_H
#define GAME_STATE_H

typedef struct GameState GameState;
struct GameState
{
	void *ctx;

	void (*OnEnter)  (void *ctx, void *state);
	void (*OnExit)   (void *ctx, void *state);
	void (*OnPause)  (void *ctx, void *state);
	void (*OnResume) (void *ctx, void *state);
	void (*OnTick)   (void *ctx, void *state, const OS_InputState *input, f32 dt, f32 elapsed);
};

#define GAME_MODE_MAX_STACK_SIZE 16

typedef struct GameStateStack GameStateStack;
struct GameStateStack
{
	GameState *stack[GAME_MODE_MAX_STACK_SIZE];
	i32 top; // -1 = empty
};

static void GameStateStackInit(GameStateStack *stack);
static void GameStateStackPush(GameStateStack *stack, GameState *mode, void *state);
static void GameStateStackPop(GameStateStack *stack, void *state);
static GameState *GameStateStackPeek(const GameStateStack *stack);
static void GameStateStackTick(const GameStateStack *stack, void *state, const OS_InputState *input, f32 dt, f32 elapsed);

#endif // GAME_STATE_H

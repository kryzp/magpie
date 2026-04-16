#ifndef GAME_MODE_H
#define GAME_MODE_H

typedef struct GameMode GameMode;
struct GameMode
{
	void *ctx;

	void (*OnEnter)  (void *ctx, void *state);
	void (*OnExit)   (void *ctx, void *state);
	void (*OnPause)  (void *ctx, void *state); // When a new mode is pushed.
	void (*OnResume) (void *ctx, void *state); // When the top mode is popped.
	void (*OnTick)   (void *ctx, void *state, f32 dt, const I_InputSt *input);
};

#define GAME_MODE_MAX_STACK_SIZE 16

typedef struct GameModeStack GameModeStack;
struct GameModeStack
{
	GameMode *stack[GAME_MODE_MAX_STACK_SIZE];
	i32 top; // -1 = empty
};

void      GameModeStackInit (GameModeStack *stack);
void      GameModeStackPush (GameModeStack *stack, GameMode *mode, void *state);
void      GameModeStackPop  (GameModeStack *stack, void *state);
GameMode *GameModeStackPeek (const GameModeStack *stack);

#endif // GAME_MODE_H

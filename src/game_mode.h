#ifndef GAME_MODE_H
#define GAME_MODE_H

typedef struct GameMode GameMode;
struct GameMode
{
	String8 name;

	void *ctx;

	void (*OnEnter)  (GameMode *mode, void *state);
	void (*OnExit)   (GameMode *mode, void *state);
	void (*OnPause)  (GameMode *mode, void *state); // When a new mode is pushed.
	void (*OnResume) (GameMode *mode, void *state); // When the top mode is popped.
	
	void (*Tick)     (GameMode *mode, void *state, f32 dt, const I_InputSt *input);
};

#define GAME_MODE_MAX_STACK_SIZE 16

typedef struct GameModeStack GameModeStack;
struct GameModeStack
{
	GameMode *stack[GAME_MODE_MAX_STACK_SIZE];
	i32 top; // -1 = empty
}

void      GameModeStackInit (GameModeStack *stack);
void      GameModeStackPush (GameModeStack *stack, GameMode *mode, void *state);
void      GameModeStackPop  (GameModeStack *stack, void *state);
GameMode *GameModeStackPeek (const GameModeStack *stack);

#endif // GAME_MODE_H

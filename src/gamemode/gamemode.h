#ifndef GAME_MODE_H
#define GAME_MODE_H

typedef struct GM_Mode GM_Mode;
struct GM_Mode
{
	void *ctx;

	void (*OnEnter)  (void *ctx, void *state); // When a mode is pushed.
	void (*OnExit)   (void *ctx, void *state); // When a mode is popped.
	void (*OnPause)  (void *ctx, void *state); // When a mode is pushed over the current one.
	void (*OnResume) (void *ctx, void *state); // When a mode is popped to the new one.
	void (*OnTick)   (void *ctx, void *state, f32 dt, const I_InputSt *input);
};

#define GAME_MODE_MAX_STACK_SIZE 16

typedef struct GM_Stack GM_Stack;
struct GM_Stack
{
	GM_Mode *stack[GAME_MODE_MAX_STACK_SIZE];
	i32 top; // -1 = empty
};

internal void     GM_StackInit (GM_Stack *stack);
internal void     GM_StackPush (GM_Stack *stack, GM_Mode *mode, void *state);
internal void     GM_StackPop  (GM_Stack *stack, void *state);
internal GM_Mode *GM_StackPeek (const GM_Stack *stack);
internal void     GM_StackTick (const GM_Stack *stack, void *state, f32 dt, const I_InputSt *input);

#endif // GAME_MODE_H

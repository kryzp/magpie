
internal void GameStateStackInit(GameStateStack *stack)
{
	stack->top = -1;
}

internal void GameStateStackPush(GameStateStack *stack, GameState *st, void *state)
{
	AssertTrue(stack->top < ArraySize(stack->stack));

	GameState *top = GameStateStackPeek(stack);
	
	if (top && top->OnPause)
		top->OnPause(top->ctx, state);
	
	stack->stack[stack->top] = st;
	stack->top++;

	top = GameStateStackPeek(stack);
	
	if (top && top->OnEnter)
		top->OnEnter(top->ctx, state);
}

internal void GameStateStackPop(GameStateStack *stack, void *state)
{
	AssertTrue(stack->top >= 0);

	GameState *top = GameStateStackPeek(stack);
	
	if (top && top->OnExit)
		top->OnExit(top->ctx, state);

	stack->top--;

	top = GameStateStackPeek(stack);

	if (top && top->OnResume)
		top->OnResume(top->ctx, state);
}

internal GameState *GameStateStackPeek(const GameStateStack *stack)
{
	if (stack->top < 0 || stack->top >= ArraySize(stack->stack))
		return NULL;

	return stack->stack[stack->top];
}

internal void GameStateStackTick(const GameStateStack *stack, void *state, const OS_InputState *input, f32 dt, f32 elapsed)
{
	GameState *top = GameStateStackPeek(stack);

	if (top && top->OnTick)
		top->OnTick(top->ctx, state, input, dt, elapsed);
}

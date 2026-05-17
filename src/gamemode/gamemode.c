
internal void
GM_StackInit(GM_Stack *stack)
{
	MemZeroStruct(stack);
	
	stack->top = -1;
}

internal void
GM_StackPush(GM_Stack *stack, GM_Mode *mode, void *state)
{
	AssertTrue(mode);
	AssertTrue(stack->top < ArraySize(stack->stack));

	GM_Mode *top = GM_StackPeek(stack);
	
	if (top && top->OnPause)
		top->OnPause(top->ctx, state);
	
	stack->stack[stack->top] = mode;
	stack->top++;

	top = GM_StackPeek(stack);
	
	if (top && top->OnEnter)
		top->OnEnter(top->ctx, state);
}

internal void
GM_StackPop(GM_Stack *stack, void *state)
{
	AssertTrue(stack->top >= 0);

	GM_Mode *top = GM_StackPeek(stack);
	
	if (top && top->OnExit)
		top->OnExit(top->ctx, state);

	stack->top--;

	top = GM_StackPeek(stack);

	if (top && top->OnResume)
		top->OnResume(top->ctx, state);
}

internal GM_Mode *
GM_StackPeek(const GM_Stack *stack)
{
	if (stack->top < 0 || stack->top >= ArraySize(stack->stack))
		return NULL;

	return stack->stack[stack->top];
}

internal void
GM_StackTick(const GM_Stack *stack, void *state, f32 dt, const OS_InputState *input)
{
	GM_Mode *top = GM_StackPeek(stack);

	if (top && top->OnTick)
		top->OnTick(top->ctx, state, dt, input);
}

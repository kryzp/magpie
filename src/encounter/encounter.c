
internal void
ENC_Init(ENC_Encounter *enc, void *st)
{
	MemZeroStruct(enc);
	enc->state = st;
}

internal void
ENC_Start(ENC_Encounter *enc)
{
	AssertTrue(enc->phase_count > 0);

	enc->active = true;
	enc->finished = false;
	enc->current_phase = 0;

	const ENC_Phase *p = ENC_GetCurrentPhase(enc);

	if (p && p->OnEnter)
		p->OnEnter(enc->state, p->ctx);
}

internal void
ENC_Stop(ENC_Encounter *enc)
{
	if (!enc->active)
		return;

	const ENC_Phase *p = ENC_GetCurrentPhase(enc);

	if (p && p->OnExit)
		p->OnExit(enc->state, p->ctx);

	enc->active = false;
}

internal void
ENC_Tick(ENC_Encounter *enc, f32 dt)
{
	if (!enc->active || enc->finished)
		return;
	
	const ENC_Phase *p = ENC_GetCurrentPhase(enc);

	if (!p)
	{
		enc->finished = true;
		enc->active = false;
		return;
	}
	
	if (p->OnTick)
		p->OnTick(enc->state, p->ctx, dt);

	if (p->IsCompleted(enc->state, p->ctx))
		ENC_Advance(enc);
}

internal void
ENC_Add(ENC_Encounter *enc, const ENC_Phase *phase)
{
	AssertTrue(enc->phase_count < ArraySize(enc->phases));

	enc->phases[enc->phase_count] = *phase;
	enc->phase_count++;
}

internal void
ENC_Advance(ENC_Encounter *enc)
{
	if (!enc->active)
		return;

	const ENC_Phase *old = ENC_GetCurrentPhase(enc);

	if (old && old->OnExit)
		old->OnExit(enc->state, old->ctx);
	
	enc->current_phase++;

	if (enc->current_phase >= enc->phase_count)
	{
		enc->finished = true;
		enc->active = false;
		return;
	}

	const ENC_Phase *new = ENC_GetCurrentPhase(enc);

	if (new && new->OnEnter)
		new->OnEnter(enc->state, new->ctx);
}

internal const ENC_Phase *
ENC_GetCurrentPhase(const ENC_Encounter *enc)
{
	if (enc->current_phase >= enc->phase_count)
		return NULL;

	return &enc->phases[enc->current_phase];
}

#ifndef ENCOUNTER_H
#define ENCOUNTER_H

typedef struct ENC_Phase ENC_Phase;
struct ENC_Phase
{
	void *ctx;

	void (*OnEnter) (void *enc_state, void *phase_ctx);
	void (*OnExit)  (void *enc_state, void *phase_ctx);
	void (*OnTick)  (void *enc_state, void *phase_ctx, f32 dt);

	b32  (*IsCompleted)(void *enc_state, void *phase_ctx);
};

typedef struct ENC_Encounter ENC_Encounter;
struct ENC_Encounter
{
	void *state;

	b32 active;
	b32 finished;
	
	ENC_Phase phases[32];
	u32 phase_count;
	
	u32 current_phase;
};

internal void ENC_Init  (ENC_Encounter *enc, void *st);
internal void ENC_Start (ENC_Encounter *enc);
internal void ENC_Stop  (ENC_Encounter *enc);
internal void ENC_Tick  (ENC_Encounter *enc, f32 dt);

internal void ENC_Add     (ENC_Encounter *enc, const ENC_Phase *phase);
internal void ENC_Advance (ENC_Encounter *enc);

internal const ENC_Phase *ENC_GetCurrentPhase(const ENC_Encounter *enc);

#endif // ENCOUNTER_H

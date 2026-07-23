#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

typedef enum PlayerStateType
{
	PlayerStateType_Moving,
	PlayerStateType_Rolling,
	PlayerStateType_COUNT
}
PlayerStateType;

typedef struct PlayerInput PlayerInput;
struct PlayerInput
{
	v2 movement;
	b32 roll;
	b32 run;

	b32 aim;
	b32 just_started_aiming;
	
	b32 fire;
	b32 reload;
};

typedef struct PlayerAnimationClips PlayerAnimationClips;
struct PlayerAnimationClips
{
	AN_ClipKey k_walk;
	AN_ClipKey k_run;
};

typedef struct PlayerAnimationState PlayerAnimationState;
struct PlayerAnimationState
{
	b32 is_grounded;
	b32 is_aiming;
	b32 is_moving;
	b32 is_running;
};

typedef struct Player Player;
struct Player
{
	E_Header base;

	Arena arena;

	PlayerStateType state;

	f32 roll_time;
	v3 roll_direction;

	v4 target_rotation;

	P_Handle rigidbody_handle;
	R_ModelInstance render_model;

	PlayerAnimationClips anim_clips;

	Gun gun;
};

internal PlayerInput PlayerGatherInput(const OS_InputState *st);

internal void PlayerAnimate(const PlayerAnimationState *st, f32 global_time, const PlayerAnimationClips *clips, AN_Animator *animator);

internal v3 CalcPlayerAimingPoint(const OS_InputState *input);

internal PlayerAnimationState PlayerTickMovement(Player *player, const E_TickContext *ctx, const PlayerInput *input_st);
internal PlayerAnimationState PlayerTickRolling(Player *player, const E_TickContext *ctx, const PlayerInput *input_st);

internal void PlayerInit(Player *player, Transform transform);
internal void PlayerDestroy(Player *player);

internal void PlayerPreAnimTick(Player *player, const E_TickContext *ctx);
internal void PlayerPostAnimTick(Player *player, const E_TickContext *ctx);
internal void PlayerPostPhysicsTick(Player *player, const E_TickContext *ctx);

internal void PlayerSerialize(Player *player, IO_ByteSerializer *writer);
internal void PlayerDeserialize(Player *player, IO_ByteSerializer *reader);

#endif // GAME_PLAYER_H

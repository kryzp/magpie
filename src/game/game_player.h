#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

typedef struct PlayerInput PlayerInput;
struct PlayerInput
{
	v2 movement;
	b32 jump;
	b32 run;
	b32 sneak;

	b32 aim;
	b32 just_started_aiming;
	
	b32 fire;
	b32 reload;
};

typedef struct PlayerAnimationState PlayerAnimationState;
struct PlayerAnimationState
{
	f32 elapsed;

	b32 is_grounded;
	b32 is_aiming;
	b32 is_running;
	b32 is_sneaking;
	b32 is_moving;
};

typedef struct Player Player;
struct Player
{
	E_Header header;

	Arena arena;

	v4 target_rotation;

	P_Handle rigidbody_handle;
	R_SceneHandle scene_object_handle;
	AN_Handle anim_handle;
	
	Gun gun;
};

static PlayerInput PlayerGatherInput(const OS_InputState *st);

static void PlayerAnimate(const PlayerAnimationState *st, AN_Handle handle);

static v3 CalcPlayerAimingPoint(const OS_InputState *input);

static void PlayerInit(Player *player, Transform transform);
static void PlayerDestroy(Player *player);

static void PlayerPreAnimTick(Player *player, const E_TickContext *ctx);
static void PlayerPostAnimTick(Player *player, const E_TickContext *ctx);
static void PlayerPostPhysicsTick(Player *player, const E_TickContext *ctx);

static void PlayerSerialize(Player *player, IO_ByteSerializer *writer);
static void PlayerDeserialize(Player *player, IO_ByteSerializer *reader);

#endif // GAME_PLAYER_H

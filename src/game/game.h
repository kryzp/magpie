#ifndef GAME_H
#define GAME_H

typedef enum GameEntityType
{
#define GameEntityDef(type, max) GameEntityType_##type,
#include "game_entity_xmacro.inc"
#undef GameEntityDef
    GameEntityType_COUNT
}
GameEntityType;

typedef enum CameraDriverMode
{
	CameraDriverMode_Unrestricted,
	CameraDriverMode_COUNT
}
CameraDriverMode;

typedef struct CameraDriverConfig CameraDriverConfig;
struct CameraDriverConfig
{
	CameraDriverMode mode;
	
	v3 offset;

	f32 fov;

	f32 yaw_min, yaw_max;
	f32 pitch_min, pitch_max;

	f32 shake_factor;
};

typedef struct CameraDriver CameraDriver;
struct CameraDriver
{
	CameraDriverConfig config;
	
	f32 yaw, target_yaw;
	f32 pitch, target_pitch;
};

static CameraDriver CameraDriverInit(const CameraDriverConfig *config);

static void CameraDriverShake(CameraDriver *driver, f32 amount);
static void CameraDriverDrive(CameraDriver *driver, R_Camera *camera, const OS_InputState *input, f32 dt);

typedef enum AmmoType
{
    AmmoType_Magnum357,
    AmmoType_COUNT
}
AmmoType;

typedef struct GunSpecs GunSpecs;
struct GunSpecs
{
    u32 max_ammo_in_clip;
    AmmoType ammo_type;
    f32 rechamber_time;
};

typedef struct Gun Gun;
struct Gun
{
    E_Transform transform;
    
    u32 ammo_count;
    GunSpecs specs;

    CH_Timer shoot_timer;
};

static void GunFire(Gun *gun);
static void GunReload(Gun *gun);

#define PLAYER_MOVE_SPEED 10.f

typedef struct PlayerInput PlayerInput;
struct PlayerInput
{
	v2 movement;
    b32 jump;

    b32 aiming;
    b32 just_started_aiming;
    
    b32 fire;
    b32 reload;
};

static PlayerInput PlayerGatherInput(const OS_InputState *input);

typedef struct Player Player;
struct Player
{
    E_Header header;

    E_Transform transform;

    Gun gun;
};

static void PlayerInit(Player *player, const E_TickContext *ctx);
static void PlayerDestroy(Player *player);
static void PlayerPreAnimTick(Player *player, const E_TickContext *ctx);
static void PlayerPostAnimTick(Player *player, const E_TickContext *ctx);
static void PlayerPostPhysicsTick(Player *player, const E_TickContext *ctx);
static void PlayerSerialize(Player *player, IO_ByteSerializer *writer);
static void PlayerDeserialize(Player *player, IO_ByteSerializer *reader);

typedef struct Game Game;
struct Game
{
    E_TID entity_types[GameEntityType_COUNT];
    E_Handle player_handle;
	GM_Stack game_mode_stack;
	R_Camera camera;
	CameraDriver camera_driver;
	b32 camera_driver_active;
};

static void GameRegisterEntities(Game *game, E_World *world);
static void GameInit(Game *game, E_World *world);
static void GameTick(Game *game, const OS_InputState *input, f32 dt, f32 elapsed);

#endif // GAME_H

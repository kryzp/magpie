
#include "game.h"
#include "chrono/chrono_timer.h"
#include "entity/entity_transform.h"
static CameraDriver CameraDriverInit(const CameraDriverConfig *config)
{
	CameraDriver driver = {0};
	driver.config = *config;

	return driver;
}

static void CameraDriverShake(CameraDriver *driver, f32 amount)
{
	// TODO
}

static void CameraDriverDrive(CameraDriver *driver, R_Camera *camera, const OS_InputState *input, f32 dt)
{
	const f32 mouse_deadzone = 0.001f;
	const f32 mouse_sens = 0.08f;
	const f32 turn_interp = 35.f;

	f32 move_speed = 2.5f;

	if (OS_KbCtrl(input))
		move_speed *= 0.25f;

	if (OS_KbAlt(input))
		move_speed *= 2.f;

	const f32 dx = input->mouse_delta.x;
	const f32 dy = input->mouse_delta.y;

	if (dx*dx + dy*dy >= mouse_deadzone*mouse_deadzone)
	{
		driver->target_pitch -= dy * mouse_sens;
		driver->target_yaw   -= dx * mouse_sens;
	}

	driver->pitch = LerpValue(driver->pitch, driver->target_pitch, turn_interp * dt);
	driver->yaw = LerpValue(driver->yaw,   driver->target_yaw,   turn_interp * dt);

	f32 corrected_pitch = driver->pitch;
	f32 corrected_yaw   = driver->yaw + MATH_PI*0.5f;

	camera->forward = V3SphericalToCartesian(1.f,
											 corrected_yaw   * MATH_DEG_TO_RAD,
											 corrected_pitch * MATH_DEG_TO_RAD);

	v3 basis[3] = {0};
	basis[0] = V3Normalize(V3Cross(camera->forward, v3(0.f, 0.f, 1.f)));
	basis[1] = camera->forward;
	basis[2] = V3Normalize(V3Cross(basis[0], basis[1]));

	for (u32 i = 0; i < 3; i++)
		basis[i] = V3MulF32(basis[i], move_speed * dt);

	f32 hori = (i32)OS_KbDown(input, OS_KeyboardKey_D)     - (i32)OS_KbDown(input, OS_KeyboardKey_A);
	f32 frwd = (i32)OS_KbDown(input, OS_KeyboardKey_W)     - (i32)OS_KbDown(input, OS_KeyboardKey_S);
	f32 vert = (i32)OS_KbDown(input, OS_KeyboardKey_Space) - (i32)OS_KbShift(input);

	camera->position = V3Add(camera->position, V3MulF32(basis[0], hori));
	camera->position = V3Add(camera->position, V3MulF32(basis[1], frwd));
	camera->position = V3Add(camera->position, V3MulF32(basis[2], vert));

	osapi->SetMouseLocked(true);
	
	R_CameraRecompute(camera);
}

static void GunFire(Gun *gun)
{
	if (CH_TimerElapsed(&gun->shoot_timer) < gun->specs.rechamber_time && gun->ammo_count != gun->specs.max_ammo_in_clip)
	{
		DebugPrintT("Rechambering... %f / %f", CH_TimerElapsed(&gun->shoot_timer), gun->specs.rechamber_time);
		return;
	}

	if (gun->ammo_count <= 0)
	{
		DebugPrintT("Click!");
		return;
	}

	gun->ammo_count--;

	DebugPrintT("Fired! %u / %u", gun->ammo_count, gun->specs.max_ammo_in_clip);

	CH_TimerRestart(&gun->shoot_timer);
}

static void GunReload(Gun *gun)
{
	gun->ammo_count = gun->specs.max_ammo_in_clip;

	DebugPrintT("Reloaded! %u / %u", gun->ammo_count, gun->specs.max_ammo_in_clip);
}

static PlayerInput PlayerGatherInput(const OS_InputState *input)
{
	PlayerInput output = {0};

	output.movement.x = (f32)OS_KbDown(input, OS_KeyboardKey_D) - (f32)OS_KbDown(input, OS_KeyboardKey_A);
	output.movement.y = (f32)OS_KbDown(input, OS_KeyboardKey_W) - (f32)OS_KbDown(input, OS_KeyboardKey_S);
	output.movement = V2Normalize(output.movement);

	output.jump = OS_KbPressed(input, OS_KeyboardKey_Space);

	output.aiming = OS_MbDown(input, OS_MouseButton_Right);
	output.just_started_aiming = OS_MbPressed(input, OS_MouseButton_Right);

	output.fire = OS_MbPressed(input, OS_MouseButton_Left) && output.aiming;
	output.reload = OS_MbPressed(input, OS_MouseButton_Left) && !output.aiming;

	return output;
}

static void PlayerInit(Player *player, const E_TickContext *ctx)
{
	GunSpecs revolver_specs = {0};
	revolver_specs.ammo_type = AmmoType_Magnum357;
	revolver_specs.max_ammo_in_clip = 6;
	revolver_specs.rechamber_time = .5f;

	player->gun.specs = revolver_specs;
}

static void PlayerDestroy(Player *player)
{
}

static void PlayerPreAnimTick(Player *player, const E_TickContext *ctx)
{
	PlayerInput input_st = PlayerGatherInput(ctx->input);

	f32 move_speed = PLAYER_MOVE_SPEED;

	if (input_st.aiming)
		move_speed *= 0.5f;
	
	if (input_st.fire)
		GunFire(&player->gun);

	if (input_st.reload)
		GunReload(&player->gun);

	v2 xy_movement_input = input_st.movement;

	v3 movement = v3x(0.f);
	movement.x = xy_movement_input.x * move_speed;
	movement.y = xy_movement_input.y * move_speed;
	movement.z = 0.f;

	E_TransformMoveBy(&player->transform, movement); // todo: replace with physics handle call
}

static void PlayerPostAnimTick(Player *player, const E_TickContext *ctx)
{
}

static void PlayerPostPhysicsTick(Player *player, const E_TickContext *ctx)
{
}

static void PlayerSerialize(Player *player, IO_ByteSerializer *writer)
{
}

static void PlayerDeserialize(Player *player, IO_ByteSerializer *reader)
{
}

static void GameRegisterEntities(Game *game, E_World *world)
{
#define GameEntityDef(type, max)										\
	{																	\
		E_TypeDesc desc = {												\
			.name              = String8Lit(STRINGIFY(type)),			\
			.stride            = sizeof(type),							\
			.max_instances     = (max),									\
			.OnInit            = (E_TypeDescTickFn *)type##Init, 		\
			.OnDestroy         = (E_TypeDescDestroyFn *)type##Destroy, \
			.OnPreAnimTick     = (E_TypeDescTickFn *)type##PreAnimTick, \
			.OnPostAnimTick    = (E_TypeDescTickFn *)type##PostAnimTick, \
			.OnPostPhysicsTick = (E_TypeDescTickFn *)type##PostPhysicsTick, \
			.OnSerialize       = (E_TypeDescSerializeFn *)type##Serialize, \
			.OnDeserialize     = (E_TypeDescDeserializeFn *)type##Deserialize \
		};																\
		game->entity_types[GameEntityType_##type] = E_WorldRegisterType(world, &desc); \
	}

#include "game_entity_xmacro.inc"

#undef GameEntityDef
}

static void GameInit(Game *game, E_World *world)
{
	GM_StackInit(&game->game_mode_stack);

	game->camera = R_CameraPerspective(v3x(0.f), v3(0.f, 1.f, 0.f), 90.f, 1280.f / 720.f, .1f, 100.f);

	CameraDriverConfig camera_driver_cfg = {0};
	camera_driver_cfg.mode = CameraDriverMode_Unrestricted;
	game->camera_driver = CameraDriverInit(&camera_driver_cfg);

	game->player_handle = E_WorldSpawn(world, game->entity_types[GameEntityType_Player], E_TransformIdentity());
}

static void GameTick(Game *game, const OS_InputState *input, f32 dt, f32 elapsed)
{
	GM_StackTick(&game->game_mode_stack, game, dt, input);
	
	CameraDriverDrive(&game->camera_driver, &game->camera, input, dt);
}

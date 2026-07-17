
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
	switch (driver->config.mode)
	{
		case CameraDriverMode_Unrestricted:
			break;

		case CameraDriverMode_Player:
		{
			Player *player = E_WorldGet(&app->world, game->player_handle);
			P_RigidBody *rb = P_GetRigidbodyFromHandle(player->rigidbody_handle);

			v3 player_position = rb->position;
			
			driver->target_look_at = v3(player_position.x, player_position.y, 0.f);

			if (OS_MbDown(input, OS_MouseButton_Right))
			{
				v3 aiming_point = CalcPlayerAimingPoint(input);
				v3 diff = V3Sub(aiming_point, rb->position);
				v3 aim_dir = V3Normalize(diff);
				f32 look_ahead_distance = 1.f - 1.f/PowF(V3Length(diff)+1.f,0.2f);
				look_ahead_distance *= 10.0f;
				driver->target_look_at = V3Add(player_position, V3MulF32(aim_dir, look_ahead_distance));
			}
			
			driver->final_offset = v3(0.f, -3.0f, 7.0f);
			
			driver->intermediate_position = V3Lerp(driver->intermediate_position, driver->target_look_at, 25.f * dt);
	
			camera->position = V3Add(driver->intermediate_position, driver->final_offset);
	
			camera->forward = V3Normalize(V3MulF32(driver->final_offset, -1.f));
		}
		break;
	}
}

/*

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
	driver->target_yaw -= dx * mouse_sens;
	driver->target_pitch -= dy * mouse_sens;
}

driver->yaw = LerpValue(driver->yaw, driver->target_yaw, turn_interp * dt);
driver->pitch = LerpValue(driver->pitch, driver->target_pitch, turn_interp * dt);

f32 corrected_yaw = driver->yaw + 90.f;
f32 corrected_pitch = driver->pitch;

camera->forward = V3SphericalToCartesian(1.f,
										 corrected_yaw * MATH_DEG_TO_RAD,
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

//camera->position = V3Add(camera->position, V3MulF32(basis[0], hori));
//camera->position = V3Add(camera->position, V3MulF32(basis[1], frwd));
//camera->position = V3Add(camera->position, V3MulF32(basis[2], vert));

osapi->SetMouseLocked(true);
	
R_CameraRecompute(camera);

*/

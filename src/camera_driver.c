
internal CameraDriver
CameraDriverInit(const CameraDriverConfig *config)
{
	CameraDriver driver = {0};
	driver.config = *config;

	return driver;
}

internal void
CameraDriverShake(CameraDriver *driver, f32 amount)
{
	// TODO
}

internal void
CameraDriverDrive(CameraDriver *driver, R_Camera *camera, const I_State *input, f32 dt)
{
	const f32 mouse_deadzone = 0.001f;
	const f32 turn_speed     = 0.1f;
	const f32 turn_interp    = 35.f;

	f32 move_speed = 2.5f;

	if (I_Ctrl(input))
		move_speed *= 0.25f;

	if (I_Alt(input))
		move_speed *= 2.f;

	const f32 dx = input->mouse_delta.x;
	const f32 dy = input->mouse_delta.y;

	if (dx*dx + dy*dy >= mouse_deadzone*mouse_deadzone)
	{
		driver->target_pitch -= dy * turn_speed;
		driver->target_yaw   -= dx * turn_speed;
	}

	driver->pitch = LerpValue(driver->pitch, driver->target_pitch, turn_interp * dt);
	driver->yaw   = LerpValue(driver->yaw,   driver->target_yaw,   turn_interp * dt);

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

	f32 hori = I_KbDown(input, I_KeyboardKey_D)     - I_KbDown(input, I_KeyboardKey_A);
	f32 frwd = I_KbDown(input, I_KeyboardKey_W)     - I_KbDown(input, I_KeyboardKey_S);
	f32 vert = I_KbDown(input, I_KeyboardKey_Space) - I_Shift(input);

	camera->position = V3Add(camera->position, V3MulF32(basis[0], hori));
	camera->position = V3Add(camera->position, V3MulF32(basis[1], frwd));
	camera->position = V3Add(camera->position, V3MulF32(basis[2], vert));

	R_CameraRecompute(camera);
}

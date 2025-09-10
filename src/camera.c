
internal void CameraRecompute(Camera *camera)
{
	camera->view = M4LookAt(camera->position,
				V3AddV3(camera->position, camera->forward),
				camera->up);

	switch (camera->type) {
	case CameraType_Perspective:
		camera->projection = M4Perspective(camera->fov, camera->aspect,
						   camera->near_plane,
						   camera->far_plane);
		break;

	case CameraType_Orthographic:
		// TODO
		break;
	}
}

internal Camera CameraInitPerspective(v3 position, v3 forward, f32 fov,
				      f32 aspect, f32 near_plane, f32 far_plane)
{
	Camera camera = {0};
	camera.type = CameraType_Perspective;
	camera.position = position;
	camera.forward = forward;
	camera.up = v3(0.f, 0.f, 1.f);
	camera.fov = fov;
	camera.aspect = aspect;
	camera.near_plane = near_plane;
	camera.far_plane = far_plane;

	CameraRecompute(&camera);

	return camera;
}

internal void CameraDriverUpdate(CameraDriver *driver, Camera *camera, f32 dt)
{
	const f32 mouse_deadzone = .001f;
	const f32 turn_speed = 1.f;
	const f32 move_speed = 2.5f;

	f32 dx = (f32)(platform->mouse_position.x - platform->window_width/2);
	f32 dy = (f32)(platform->mouse_position.y - platform->window_height/2);

	if (dx*dx + dy*dy > mouse_deadzone*mouse_deadzone) {
		driver->target_yaw   -= dx * turn_speed * dt;
		driver->target_pitch -= dy * turn_speed * dt;
	}

	driver->yaw   += (driver->target_yaw   - driver->yaw)*dt*50.f;
	driver->pitch += (driver->target_pitch - driver->pitch)*dt*50.f;

	f32 yaw   = driver->yaw + PIf*.5f;
	f32 pitch = driver->pitch;

	camera->forward = SphericalToCartesian(1.f, yaw, pitch);

	v3 basis[3] = {0};
	basis[0] = V3Normalize(V3Cross(camera->forward, v3(0.f, 0.f, 1.f)));
	basis[1] = camera->forward;
	basis[2] = V3Normalize(V3Cross(basis[0], camera->forward));

	for (u32 i = 0; i < 3; i++)
		basis[i] = V3MultiplyF32(basis[i], move_speed * dt);
	
	if (platform->kb_down[KeyboardKey_D])
		camera->position = V3AddV3(camera->position, basis[0]);
	else if (platform->kb_down[KeyboardKey_A])
		camera->position = V3SubV3(camera->position, basis[0]);

	if (platform->kb_down[KeyboardKey_W])
		camera->position = V3AddV3(camera->position, basis[1]);
	else if (platform->kb_down[KeyboardKey_S])
		camera->position = V3SubV3(camera->position, basis[1]);
	
	if (platform->kb_down[KeyboardKey_Space])
		camera->position = V3AddV3(camera->position, basis[2]);
	else if (platform->kb_down[KeyboardKey_LeftShift] || platform->kb_down[KeyboardKey_RightShift])
		camera->position = V3SubV3(camera->position, basis[2]);

	CameraRecompute(camera);

	platform->SetMousePosition(platform->window_width / 2,
				   platform->window_height / 2);
}

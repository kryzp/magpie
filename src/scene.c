
internal void
CameraRecompute(Camera *camera)
{
	camera->view = M4LookAt(camera->position,
							V3AddV3(camera->position, camera->forward),
							camera->up);
	
	switch(camera->type)
	{
		case CameraType_Perspective:
		{
			camera->projection = M4Perspective(camera->fov,
											   camera->aspect,
											   camera->near_plane,
											   camera->far_plane);
		}
		break;
		
		case CameraType_Orthographic:
		{
			// TODO(kp)
		}
		break;
	}
	
	camera->dirty = false;
}

internal Camera
CameraInitPerspective(v3 position, v3 forward,
					  f32 fov, f32 aspect,
					  f32 near_plane, f32 far_plane)
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

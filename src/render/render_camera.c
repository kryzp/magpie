
static R_Camera R_CameraPerspective(v3 position, v3 forward, f32 fov, f32 aspect, f32 near, f32 far)
{
	R_Camera camera = {0};
	camera.type = R_CameraType_Perspective;
	camera.position = position;
	camera.forward = forward;
	camera.up = v3(0.f, 0.f, 1.f);
	camera.fov = fov;
	camera.aspect = aspect;
	camera.near_plane = near;
	camera.far_plane = far;

	R_CameraRecompute(&camera);

	return camera;
}

static R_Camera R_CameraOrthographic(v3 position, v3 forward, v4 rect, f32 near, f32 far)
{
	R_Camera camera = {0};
	camera.type = R_CameraType_Orthographic;
	camera.position = position;
	camera.forward = forward;
	camera.up = v3(0.f, 0.f, 1.f);
	camera.ortho = rect;
	camera.near_plane = near;
	camera.far_plane = far;

	R_CameraRecompute(&camera);

	return camera;
}

static void R_CameraRecompute(R_Camera *camera)
{
	camera->view = M4LookAt(camera->position,
							V3Add(camera->position, camera->forward),
							camera->up);

	switch (camera->type)
	{
		case R_CameraType_Perspective:
			camera->proj = M4Perspective(camera->fov, camera->aspect, camera->near_plane, camera->far_plane);
			break;

		case R_CameraType_Orthographic:
			camera->proj = M4Orthographic(camera->ortho.x, camera->ortho.x + camera->ortho.width,
										  camera->ortho.y, camera->ortho.y + camera->ortho.height,
										  camera->near_plane, camera->far_plane);
			break;
	}
}

static R_FrustumVolume R_CameraFrustum(const R_Camera *camera)
{
	m4 vpt = M4Transpose(M4MulM4(camera->proj, camera->view));

	R_FrustumVolume volume = {0};

	volume.planes[0] = V4FrustumNormalizePlane(V4Add(vpt.c[3], vpt.c[0]));
	volume.planes[1] = V4FrustumNormalizePlane(V4Sub(vpt.c[3], vpt.c[0]));
	volume.planes[2] = V4FrustumNormalizePlane(V4Add(vpt.c[3], vpt.c[1]));
	volume.planes[3] = V4FrustumNormalizePlane(V4Sub(vpt.c[3], vpt.c[1]));
	volume.planes[4] = V4FrustumNormalizePlane(                vpt.c[2]);
	volume.planes[5] = V4FrustumNormalizePlane(V4Sub(vpt.c[3], vpt.c[2]));

	return volume;
}

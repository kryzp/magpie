
internal void CameraRecompute(Camera *camera)
{
	camera->view = M4LookAt(camera->position,
				V3AddV3(camera->position, camera->forward),
				camera->up);

	switch (camera->type) {
	case CameraType_Perspective: {
		camera->projection = M4Perspective(camera->fov, camera->aspect,
						   camera->near_plane,
						   camera->far_plane);
	} break;

	case CameraType_Orthographic: {
		// TODO(kp)
	} break;
	}
}

internal Camera CameraInitPerspective(v3 position, v3 forward, f32 fov,
				      f32 aspect, f32 near_plane, f32 far_plane)
{
	Camera camera = { 0 };
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

internal EnvironmentProbe EnvironmentProbeInit()
{
	EnvironmentProbe probe = { 0 };

	probe.irradiance = ImageAllocCubemap(32, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
	probe.prefilter = ImageAllocCubemap(128, VK_FORMAT_R32G32B32A32_SFLOAT, 4);

	return probe;
}

internal void SceneInit(Scene *scene)
{
}

internal void SceneDestroy(Scene *scene)
{
}

internal SceneObject *SceneObjectFromHandle(Scene *scene, u32 handle)
{
	for (i32 i = 0; i < scene->object_count; i++) {
		if (scene->objects[i].id == handle) {
			return scene->objects + i;
		}
	}

	return 0;
}

internal void SceneResolveAdding(Scene *scene)
{
	for (i32 i = 0; i < scene->pending_addition_count; i++) {
		scene->objects[scene->object_count++] =
			scene->pending_addition[i];
	}

	scene->pending_addition_count = 0;
}

internal void SceneResolveRemoving(Scene *scene)
{
	// NOTE(kp): We need to remove these from both objects and direct_batches.
	for (i32 i = 0; i < scene->pending_removal_count; i++) {
		u32 to_remove = scene->pending_removal[i];
		scene->reusable_handles[scene->reusable_handle_count++] =
			to_remove;
		scene->object_count--;
	}

	scene->pending_removal_count = 0;
}

internal void SceneRemoveSceneObject(Scene *scene, u32 handle)
{
	scene->pending_removal[scene->pending_removal_count++] = handle;
}

internal u32 SceneRegisterObject(Scene *scene, RenderContext *render_context,
				 Assets *assets, Mesh *mesh, Material *material,
				 m4 transform,
				 //Bounds3D bounds,
				 SceneObjectFlags flags)
{
	u32 handle = scene->object_count + scene->pending_addition_count;

	if (scene->reusable_handle_count > 0) {
		handle =
			scene->reusable_handles[--scene->reusable_handle_count];
	}

	SceneObject *object = scene->objects + handle;
	object->id = handle;
	object->mesh_id = RenderContextUploadMesh(render_context, mesh);
	object->material_id =
		RenderContextUploadMaterial(render_context, assets, material);
	object->transform = transform;
	//object->bounds = bounds;
	object->flags = flags;
	//object->custom_sort_key = custom_sort_key;

	scene->pending_addition[scene->pending_addition_count++] = *object;

	return handle;
}

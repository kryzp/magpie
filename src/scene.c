
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

internal EnvironmentProbe EnvironmentProbeInit()
{
	EnvironmentProbe probe = {0};

	probe.irradiance = ImageAllocCubemap(32, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
	probe.prefilter = ImageAllocCubemap(128, VK_FORMAT_R32G32B32A32_SFLOAT, 4);

	return probe;
}

internal void SceneObjectInit(SceneObject *object)
{
	object->id          = SCENE_INVALID_HANDLE;
	object->mesh_id     = SCENE_INVALID_HANDLE;
	object->material_id = SCENE_INVALID_HANDLE;
	object->light_id    = SCENE_INVALID_HANDLE;
	object->transform   = m4(1.f);
	object->flags       = 0;
}

internal void SceneInit(Scene *scene, MemoryArena *arena)
{
	scene->arena = arena;
	
	scene->object_count = 0;
	scene->objects = NULL;
}

internal void SceneDestroy(Scene *scene)
{
}

internal SceneObject *SceneObjectFromHandle(Scene *scene, u32 handle)
{
	for (SceneObject *s = scene->objects; s; s = s->next) {
		if (s->id == handle)
			return s;
	}

	return NULL;
}

internal void SceneResolveRemoving(Scene *scene)
{
	for (i32 i = 0; i < scene->pending_removal_count; i++) {
		u32 to_remove_handle = scene->pending_removal[i];
		SceneObject *to_remove = SceneObjectFromHandle(scene, to_remove_handle);
		to_remove->next = scene->first_free_object;
		scene->first_free_object = to_remove;
		scene->reusable_handles[scene->reusable_handle_count++] = to_remove_handle;
		scene->object_count--;
	}

	scene->pending_removal_count = 0;
}

internal void SceneRemoveSceneObject(Scene *scene, u32 handle)
{
	scene->pending_removal[scene->pending_removal_count++] = handle;
}

internal u32 SceneRegisterObject(Scene *scene, m4 transform)
{
	Assert(scene->object_count < SCENE_MAX_OBJECTS &&
	       "Reached scene object limit.");
	
	u32 handle = scene->object_count;

	if (scene->reusable_handle_count > 0)
		handle = scene->reusable_handles[--scene->reusable_handle_count];

	SceneObject *object = scene->first_free_object;

	if (object) {
		scene->first_free_object = scene->first_free_object->next;
		MemoryZeroStruct(object);
	} else {
		object = MemoryArenaPush(scene->arena, sizeof(SceneObject));
	}
	
	SceneObjectInit(object);
	
	object->id = handle;
	object->transform = transform;

	object->next = scene->objects;
	scene->objects = object;
	
	scene->object_count++;

	return handle;
}

internal void SceneObjectAddMesh(Scene *scene, u32 handle,
				 RenderState *rs,
				 Assets *assets,
				 Mesh *mesh,
				 Material *material,
				 //Bounds3D bounds,
				 SceneObjectFlags flags)
{
	SceneObject *object = SceneObjectFromHandle(scene, handle);

	object->mesh_id     = RenderStateUploadMesh(rs, mesh);
	object->material_id = RenderStateUploadMaterial(rs, assets, material);

	object->flags = flags;
}

internal void SceneObjectAddLight(Scene *scene, u32 handle,
				  RenderState *rs,
				  Light *light)
{
	SceneObject *object = SceneObjectFromHandle(scene, handle);

	object->light_id = RenderStateMakeLight(rs, light);
}

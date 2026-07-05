
static PlayerInput PlayerGatherInput(const OS_InputState *st)
{
	PlayerInput input = {0};

	input.movement.x = (f32)OS_KbDown(st, OS_KeyboardKey_D) - (f32)OS_KbDown(st, OS_KeyboardKey_A);
	input.movement.y = (f32)OS_KbDown(st, OS_KeyboardKey_W) - (f32)OS_KbDown(st, OS_KeyboardKey_S);
	input.movement = V2Normalize(input.movement);

	input.jump = OS_KbPressed(st, OS_KeyboardKey_Space);

	input.run = OS_KbShift(st);
	input.sneak = OS_KbCtrl(st) && !input.run;

	input.aim = OS_MbDown(st, OS_MouseButton_Right) && !input.run;
	input.just_started_aiming = OS_MbPressed(st, OS_MouseButton_Right) && !input.run;

	input.fire = OS_MbPressed(st, OS_MouseButton_Left) && input.aim && !input.run;
	input.reload = OS_MbPressed(st, OS_MouseButton_Left) && !input.aim;

	return input;
}

static void PlayerAnimate(const PlayerAnimationState *st, AN_Handle handle)
{
	//AN_ClipKey key = AN_ClipKeyNull();
	//AN_Play(&app->animation_system, handle, key, false, st->elapsed);
}

static v3 CalcPlayerAimingPoint(const OS_InputState *input)
{
	R_Camera *camera = &game->camera;

	R_CameraRecompute(camera);

	v2 ndc = V2ScreenToNDC(input->mouse_position);

	v3 pos = camera->position;
	v3 dir = R_CameraNDCToWsRayDirection(camera, ndc);

	f32 t = -pos.z / dir.z;

	v3 result = V3Add(pos, V3MulF32(dir, t));

	return result;
}

static void PlayerInit(Player *player, Transform transform)
{
	player->rigidbody_handle = P_LeaseInstance();

	P_RigidBody *rb = P_GetRigidbodyFromHandle(player->rigidbody_handle);
	rb->solid = false;
	rb->fixed_position = false;
	rb->friction = 25.f;
	rb->air_friction = 0.5f;
	rb->gravity_factor = 10.f;
	rb->max_speed = 100.f;
	
	A_Handle model_handle = A_Require(String8Lit("assets://models/DamagedHelmet/glTF/DamagedHelmet.gltf"), A_Type_Model);

	ScratchArena scratch = ScratchBegin(NULL, 0);
	{
		G_CmdBuffer cmd = G_DeviceSubmitImBegin();
		R_ModelImportReceipt receipt = R_SceneImportModel(&app->scene, &cmd, scratch.arena, model_handle, (u32)(-1));
		G_DeviceSubmitImEnd(&cmd);

		R_ModelImportEntry *entry = &receipt.entries[0];

		R_ObjectDesc desc = {0};
		desc.transform = entry->transform;
		desc.sphere_bounds = entry->sphere_bounds;
		desc.mesh = entry->mesh;
		desc.material = entry->material;

		player->scene_object_handle = R_SceneGraphObjectCreate(&app->scene.graph, &desc);
	}
	ScratchRelease(&scratch);

	//player->anim_handle = AN_SystemCreateInstance(ctx->animation, A_HandleNull());

	GunSpecs revolver_specs = {0};
	revolver_specs.ammo_type = AmmoType_Magnum357;
	revolver_specs.max_ammo_in_clip = 6;
	revolver_specs.rechamber_time = .5f;

	player->gun.specs = revolver_specs;

	GunReload(&player->gun);
}

static void PlayerDestroy(Player *player)
{
	P_ReturnInstance(player->rigidbody_handle);
}

static void PlayerPreAnimTick(Player *player, const E_TickContext *ctx)
{
	PlayerInput input_st = PlayerGatherInput(ctx->input);

	P_RigidBody *rb = P_GetRigidbodyFromHandle(player->rigidbody_handle);

	f32 move_speed = 30.f;

	if (input_st.aim)
	{
		move_speed *= 0.5f;

		v3 source = rb->position;
		v3 destination = CalcPlayerAimingPoint(ctx->input);

		player->target_rotation = V4QuatLookAt(source, destination);
	}
	else if (!WithinEpsilon(V2LengthSqr(input_st.movement)))
	{
		v3 source = rb->position;
		v3 destination = V3Add(source, v3(input_st.movement.x, input_st.movement.y, 0.f));

		player->target_rotation = V4QuatLookAt(source, destination);
	}

	if (input_st.jump)
		rb->velocity.z += 50.f;

	if (input_st.run)
		move_speed *= 2.f;

	if (input_st.sneak)
		move_speed *= 0.25f;
	
	if (input_st.fire)
		GunFire(&player->gun);

	if (input_st.reload)
		GunReload(&player->gun);

	v3 movement = v3x(0.f);
	movement.x = input_st.movement.x * move_speed;
	movement.y = input_st.movement.y * move_speed;
	movement.z = 0.f;

	rb->acceleration = movement;
	rb->orientation = V4QuatSlerp(rb->orientation, player->target_rotation, ctx->dt * 50.f);

	PlayerAnimationState animation_st = {0};
	animation_st.elapsed = ctx->elapsed;
	animation_st.is_grounded = true;
	animation_st.is_aiming = input_st.aim || input_st.fire;
	animation_st.is_running = input_st.run;
	animation_st.is_sneaking = input_st.sneak;
	animation_st.is_moving = WithinEpsilon(V3LengthSqr(movement));

	PlayerAnimate(&animation_st, player->anim_handle);
}

static void PlayerPostAnimTick(Player *player, const E_TickContext *ctx)
{
	//AN_Animator *animator = AN_SystemGetAnimator(ctx->animation, player->anim_handle);
	//AN_Palette palette = AN_AnimatorPalette(animator, 0);
	//R_SceneObjectSetSkinning(ctx->render_scene, player->scene_object_handle, &palette);
}

static void PlayerPostPhysicsTick(Player *player, const E_TickContext *ctx)
{
	P_RigidBody *rb = P_GetRigidbodyFromHandle(player->rigidbody_handle);

	m4 final_matrix = M4Identity();
	final_matrix = M4MulM4(M4RotateAxis(MATH_PIf * 0.5f, v3(1.f, 0.f, 0.f)), final_matrix);
	final_matrix = M4MulM4(M4RotateAxis(MATH_PIf, v3(0.f, 0.f, 1.f)), final_matrix);
	final_matrix = M4MulM4(M4Transform(rb->position, rb->orientation, v3x(1.f), v3x(0.f)), final_matrix);
	
	R_SceneGraphObjectSetTransform(&app->scene.graph, player->scene_object_handle, final_matrix);
}

static void PlayerSerialize(Player *player, IO_ByteSerializer *writer)
{
}

static void PlayerDeserialize(Player *player, IO_ByteSerializer *reader)
{
}

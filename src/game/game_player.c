
static PlayerInput PlayerGatherInput(const OS_InputState *st)
{
	PlayerInput input = {0};

	input.movement.x = (f32)OS_KbDown(st, OS_KeyboardKey_D) - (f32)OS_KbDown(st, OS_KeyboardKey_A);
	input.movement.y = (f32)OS_KbDown(st, OS_KeyboardKey_W) - (f32)OS_KbDown(st, OS_KeyboardKey_S);
	input.movement = V2Normalize(input.movement);

	input.roll = OS_KbPressed(st, OS_KeyboardKey_Space);

	input.run = OS_KbShift(st);

	input.aim = OS_MbDown(st, OS_MouseButton_Right) && !input.run;
	input.just_started_aiming = OS_MbPressed(st, OS_MouseButton_Right) && !input.run;

	input.fire = OS_MbPressed(st, OS_MouseButton_Left) && input.aim && !input.run;
	input.reload = OS_MbPressed(st, OS_MouseButton_Left) && !input.aim;

	return input;
}

static void PlayerAnimate(const PlayerAnimationState *st, f32 global_time, const PlayerAnimationClips *clips, AN_Animator *animator)
{
	if (!st->is_moving)
	{
		AN_AnimatorPauseAndReset(animator, global_time);
		return;
	}

	if (st->is_running)
	{
		AN_AnimatorPlay(animator, clips->k_run, true, global_time);
		return;
	}
	else
	{
		AN_AnimatorPlay(animator, clips->k_walk, true, global_time);
		return;
	}
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
	player->arena = ArenaAlloc(Megabytes(1));

	player->state = PlayerStateType_Moving;

	player->target_rotation = V4QuatIdentity();

	player->rigidbody_handle = P_LeaseInstance();

	P_RigidBody *rb = P_GetRigidbodyFromHandle(player->rigidbody_handle);
	rb->solid = false;
	rb->fixed_position = false;
	rb->friction = 5.f;
	rb->air_friction = 0.05f;
	rb->gravity_factor = 10.f;
	
	{
		A_Handle model_handle = A_Require(String8Lit("assets://player/scene.gltf"), A_Type_Model);
		G_CmdBuffer cmd = G_DeviceSubmitImBegin();
		player->render_model = R_SceneModelCreate(&app->scene, &cmd, &player->arena, model_handle, true);
		G_DeviceSubmitImEnd(&cmd);
	}

	GunSpecs revolver_specs = {0};
	revolver_specs.ammo_type = AmmoType_Magnum357;
	revolver_specs.max_ammo_in_clip = 6;
	revolver_specs.rechamber_time = .5f;

	player->gun.specs = revolver_specs;

	GunReload(&player->gun);

	AN_Animator *animator = AN_SystemGetAnimator(player->render_model.animator_handle);
	player->anim_clips.k_walk = AN_AnimatorFindClipByName(animator, String8Lit("walk"));
	player->anim_clips.k_run = AN_AnimatorFindClipByName(animator, String8Lit("run"));
}

static void PlayerDestroy(Player *player)
{
	R_SceneModelDestroy(&app->scene, &player->render_model);
	P_ReturnInstance(player->rigidbody_handle);
}

static PlayerAnimationState PlayerTickMovement(Player *player, const E_TickContext *ctx, const PlayerInput *input_st)
{
	P_RigidBody *rb = P_GetRigidbodyFromHandle(player->rigidbody_handle);

	f32 move_speed = 1.f;
	f32 max_speed = 8.f;

	if (input_st->aim)
	{
		move_speed *= 0.5f;
		max_speed *= 0.5f;

		v3 source = rb->position;
		v3 destination = CalcPlayerAimingPoint(ctx->input);

		player->target_rotation = V4QuatLookAt(source, destination);
	}
	
	if (!WithinEpsilon(V2LengthSqr(input_st->movement)))
	{
		v3 source = rb->position;
		v3 destination = V3Add(source, v3(input_st->movement.x, input_st->movement.y, 0.f));

		if (!input_st->aim)
		{
			player->target_rotation = V4QuatLookAt(source, destination);
		}

		if (input_st->roll)
		{
			player->state = PlayerStateType_Rolling;
			player->roll_time = .22f;
			player->roll_direction = V3Normalize(V3Sub(destination, source));
		}
	}

	if (input_st->run)
	{
		max_speed *= 2.f;
	}

	if (input_st->fire)
		GunFire(&player->gun);

	if (input_st->reload)
		GunReload(&player->gun);

	v3 movement = v3x(0.f);
	movement.x = input_st->movement.x * move_speed;
	movement.y = input_st->movement.y * move_speed;
	movement.z = 0.f;

	rb->max_speed = max_speed;
	rb->acceleration = movement;
	rb->orientation = V4QuatSlerp(rb->orientation, player->target_rotation, ctx->dt * 50.f);
	
	PlayerAnimationState animation_st = {0};
	animation_st.is_grounded = true;
	animation_st.is_aiming = input_st->aim || input_st->fire;
	animation_st.is_running = input_st->run;
	animation_st.is_moving = !WithinEpsilon(V2LengthSqr(input_st->movement));
	
	return animation_st;
}

static PlayerAnimationState PlayerTickRolling(Player *player, const E_TickContext *ctx, const PlayerInput *input_st)
{
	P_RigidBody *rb = P_GetRigidbodyFromHandle(player->rigidbody_handle);

	rb->acceleration = V3MulF32(player->roll_direction, 3.f);
	rb->orientation = V4QuatLookAt(v3x(0.f), player->roll_direction);
	rb->max_speed = 14.f;

	player->roll_time -= ctx->dt;

	if (player->roll_time <= 0.f)
		player->state = PlayerStateType_Moving;

	PlayerAnimationState animation_st = {0};
	animation_st.is_grounded = true;
	animation_st.is_aiming = false;
	animation_st.is_running = false;
	animation_st.is_moving = true;
	
	return animation_st;
}

static void PlayerPreAnimTick(Player *player, const E_TickContext *ctx)
{
	PlayerInput input_st = PlayerGatherInput(ctx->input);
	PlayerAnimationState animation_st = {0};

	switch (player->state)
	{
		case PlayerStateType_Moving:
			animation_st = PlayerTickMovement(player, ctx, &input_st);
			break;

		case PlayerStateType_Rolling:
			animation_st = PlayerTickRolling(player, ctx, &input_st);
			break;
	}

	AN_Animator *animator = AN_SystemGetAnimator(player->render_model.animator_handle);
	PlayerAnimate(&animation_st, ctx->elapsed, &player->anim_clips, animator);
}

static void PlayerPostAnimTick(Player *player, const E_TickContext *ctx)
{
	R_SceneModelUpdateSkinning(&app->scene, &player->render_model);
}

static void PlayerPostPhysicsTick(Player *player, const E_TickContext *ctx)
{
	P_RigidBody *rb = P_GetRigidbodyFromHandle(player->rigidbody_handle);

	m4 final_matrix = M4Identity();
	final_matrix = M4MulM4(M4RotateAxis(MATH_PIf, v3(0.f, 0.f, 1.f)), final_matrix);
	final_matrix = M4MulM4(M4Transform(rb->position, rb->orientation, v3x(1.f), v3x(0.f)), final_matrix);
	
	R_SceneModelSetRootTransform(&app->scene, &player->render_model, final_matrix);
}

static void PlayerSerialize(Player *player, IO_ByteSerializer *writer)
{
}

static void PlayerDeserialize(Player *player, IO_ByteSerializer *reader)
{
}

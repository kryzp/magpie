
static PlayerInput PlayerGatherInput(const OS_InputState *st)
{
	PlayerInput input = {0};

	input.movement.x = (f32)OS_KbDown(st, OS_KeyboardKey_D) - (f32)OS_KbDown(st, OS_KeyboardKey_A);
	input.movement.y = (f32)OS_KbDown(st, OS_KeyboardKey_W) - (f32)OS_KbDown(st, OS_KeyboardKey_S);
	input.movement = V2Normalize(input.movement);

	input.jump = OS_KbPressed(st, OS_KeyboardKey_Space);

	input.sneak = OS_KbShift(st);

	input.aiming = OS_MbDown(st, OS_MouseButton_Right);
	input.just_started_aiming = OS_MbPressed(st, OS_MouseButton_Right);

	input.fire = OS_MbPressed(st, OS_MouseButton_Left) && input.aiming;
	input.reload = OS_MbPressed(st, OS_MouseButton_Left) && !input.aiming;

	return input;
}

static void PlayerAnimate(const PlayerAnimationState *st, AN_System *s, AN_Handle handle)
{
	AN_ClipKey key = AN_ClipKeyNull();
	AN_Play(s, handle, key, false, st->elapsed);
}

static void PlayerInit(Player *player, const E_TickContext *ctx)
{
	player->arena = ArenaAlloc(Kilobytes(512));

	A_Handle model_handle = A_Require(ctx->assets, String8Lit("assets://models/DamagedHelmet/glTF/DamagedHelmet.gltf"), A_Type_Model);

	ScratchArena scratch = ScratchBegin(NULL, 0);
	{
		G_CmdBuffer cmd = G_DeviceSubmitImBegin(ctx->graphics_device);
		R_ModelImportReceipt receipt = R_SceneImportModel(ctx->render_scene, &cmd, scratch.arena, model_handle, (u32)(-1));
		G_DeviceSubmitImEnd(ctx->graphics_device, &cmd);

		for (u32 i = 0; i < receipt.count; i++)
		{
			R_ModelImportEntry *entry = &receipt.entries[i];

			R_ObjectDesc desc = {0};
			desc.transform = entry->transform;
			desc.sphere_bounds = entry->sphere_bounds;
			desc.mesh = entry->mesh;
			desc.material = entry->material;

			player->scene_object_handle = R_SceneObjectCreate(ctx->render_scene, &desc);
		}
	}
	ScratchRelease(&scratch);

	//player->anim_handle = AN_SystemCreateInstance(ctx->animation, A_HandleNull());

	GunSpecs revolver_specs = {0};
	revolver_specs.ammo_type = AmmoType_Magnum357;
	revolver_specs.max_ammo_in_clip = 6;
	revolver_specs.rechamber_time = .5f;

	player->gun.specs = revolver_specs;
}

static void PlayerDestroy(Player *player)
{
	ArenaRelease(&player->arena);
}

static void PlayerPreAnimTick(Player *player, const E_TickContext *ctx)
{
	PlayerInput input_st = PlayerGatherInput(ctx->input);

	f32 move_speed = PLAYER_MOVE_SPEED * 0.001f;

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

	E_TransformMoveBy(E_TransformOf(player), movement); // todo: replace with physics handle call

	PlayerAnimationState animation_st = {0};
	animation_st.elapsed = ctx->elapsed;
	animation_st.is_grounded = true;
	animation_st.is_aiming = input_st.aiming || input_st.fire;
	animation_st.is_sneaking = input_st.sneak;
	animation_st.is_moving = V3LengthSqr(movement) <= MATH_EPSILON_F32;

	//PlayerAnimate(&animation_st, ctx->animation, player->anim_handle);
}

static void PlayerPostAnimTick(Player *player, const E_TickContext *ctx)
{
	m4 final_matrix = M4RotateAxis(MATH_PIf * 0.5f, v3(1.f, 0.f, 0.f));
	final_matrix = M4MulM4(E_TransformMatrix(E_TransformOf(player)), final_matrix);
	
	R_SceneObjectSetTransform(ctx->render_scene, player->scene_object_handle, final_matrix);

	//AN_Animator *animator = AN_SystemGetAnimator(ctx->animation, player->anim_handle);
	//AN_Palette palette = AN_AnimatorPalette(animator, 0);
	//R_SceneObjectSetSkinning(ctx->render_scene, player->scene_object_handle, &palette);
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

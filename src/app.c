
/*
 * At the willow tree grows the wallflower,
 * pale and cold,
 * from dust, alone.
 *
 * To the voices outside unresponsive,
 * blossoming only for the wind,
 * and to the touch, you are cold.
 *
 * Of no mother, nor child,
 * to bear your name,
 * to free your spirit.
 *
 * And I will meet you there,
 * But I have one final question,
 * what is it, you fear most?
 */

static S_BINDING_DEF(S_BND_WaitSeconds)
{
	f32 s = S_CtxGetArgF32(ctx, 0); 
	S_CtxYieldTime(ctx, s);
}

static S_BINDING_DEF(S_BND_WaitSignal)
{
	String8 sig = S_CtxGetArgStr(ctx, 0);
	S_CtxYieldSignal(ctx, sig);
}

static S_BINDING_DEF(S_BND_DebugLog)
{
	String8 msg = S_CtxGetArgStr(ctx, 0);
	
	DebugLogD(app->log_channel, "%.*s", String8VArg(msg));
}

static void AppInitScripting()
{
	app->scripting_system = S_Init(&app->scripting_arena, app->scripting_log_channel);

	S_BindGlobal(app->scripting_system, String8Lit("wait_seconds"), S_BND_WaitSeconds);
	S_BindGlobal(app->scripting_system, String8Lit("wait_signal"), S_BND_WaitSignal);
	S_BindGlobal(app->scripting_system, String8Lit("debug_log"), S_BND_DebugLog);
}

static void AppInit_()
{
	app->log_channel = osapi->LogChannelOpen(String8Lit("APP"));
	app->scripting_log_channel = osapi->LogChannelOpen(String8Lit("SCRIPT"));
	app->graphics_log_channel = osapi->LogChannelOpen(String8Lit("GRAPHICS"));
	app->audio_log_channel = osapi->LogChannelOpen(String8Lit("AUDIO"));
	app->asset_log_channel = osapi->LogChannelOpen(String8Lit("ASSETS"));
	app->animation_log_channel = osapi->LogChannelOpen(String8Lit("ANIMATION"));
	app->render_log_channel = osapi->LogChannelOpen(String8Lit("RENDER"));
	app->physics_log_channel = osapi->LogChannelOpen(String8Lit("PHYSICS"));
	app->entity_log_channel = osapi->LogChannelOpen(String8Lit("ENTITY"));
	
	AppInitScripting();

	G_DeviceInit(&app->graphics_device, &app->graphics_arena, app->graphics_log_channel);
	app->swapchain = G_DeviceSwapchainCreate(&app->graphics_device);
	G_ShaderCompilerInit(&app->shader_compiler, osapi->LogChannelOpenFrom(app->graphics_log_channel, String8Lit("SLANG")));

	app->audio_backend = AU_BackendInit(&app->audio_arena, osapi->LogChannelOpenFrom(app->audio_log_channel, String8Lit("MINI")));
	AU_Init(&app->audio_system,
			&app->audio_arena,
			app->audio_log_channel,
			app->audio_backend);
	
	A_Init(&app->assets,
		   &app->asset_arena,
		   app->asset_log_channel,
		   &app->graphics_device,
		   &app->shader_compiler,
		   app->audio_backend,
		   app->scripting_system);
	
	//A_Mount(&app->assets, String8Lit("engine://shaders"), String8Lit("src/render/shaders"));
	A_Mount(&app->assets, String8Lit("assets://"), String8Lit("res"));

	AN_SystemInit(&app->animation_system, app->animation_log_channel, &app->assets);

	G_BufferAllocInfo ring_buffer_alloc_info = {0};
	ring_buffer_alloc_info.size = Megabytes(512);
	ring_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ring_buffer_alloc_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;

	app->frame_upload_ring_buffer = G_RingBufferAlloc(&app->graphics_device, &ring_buffer_alloc_info);
	
	R_GraphInit(&app->graph, &app->render_arena, &app->graphics_device, osapi->LogChannelOpenFrom(app->render_log_channel, String8Lit("GRAPH")));
	R_SceneInit(&app->scene, &app->render_arena, &app->graphics_device, &app->assets, osapi->LogChannelOpenFrom(app->render_log_channel, String8Lit("SCENE")));
	
	R_SystemInit(&app->render_system, &app->render_arena, &app->graphics_device, &app->assets, osapi->LogChannelOpenFrom(app->render_log_channel, String8Lit("SYSTEM")));
	R_SystemGenerateLookupsAndMaps(&app->render_system, &app->graph, &app->frame_arena);

	P_EngineInit(&app->physics_engine, &app->physics_arena, app->physics_log_channel);
	
	E_WorldInit(&app->world, &app->entity_arena, osapi->LogChannelOpenFrom(app->entity_log_channel, String8Lit("WORLD")));
	E_EventQueueInit(&app->events, osapi->LogChannelOpenFrom(app->entity_log_channel, String8Lit("EVENT")));

	GameSelect(&app->game);
	GameInit(&app->world);

	CH_TimerStart(&app->elapsed_timer);
	CH_TimerStart(&app->delta_timer);
	CH_TimerStart(&app->hot_reload_timer);
}

__declspec(dllexport)
App *MagpieInit(const OS_API *api)
{
	osapi = api;

	Arena bootstrap = ArenaAlloc(sizeof(App));
	app = ArenaPushArray(&bootstrap, App, 1);
	
	app->bootstrap_arena = bootstrap;

	app->scripting_arena = ArenaAlloc(Gigabytes(1));
	app->graphics_arena  = ArenaAlloc(Gigabytes(3));
	app->audio_arena     = ArenaAlloc(Gigabytes(1));
	app->asset_arena     = ArenaAlloc(Gigabytes(3));
	app->animation_arena = ArenaAlloc(Gigabytes(1));
	app->render_arena    = ArenaAlloc(Gigabytes(2));
	app->physics_arena   = ArenaAlloc(Gigabytes(1));
	app->entity_arena    = ArenaAlloc(Gigabytes(1));
	app->frame_arena     = ArenaAlloc(Gigabytes(1));
	
	AppInit_();

	DebugLogI(app->log_channel, "Initialized.");
	
	return app;
}

__declspec(dllexport)
void MagpieDestroy(App *app_)
{
	G_DeviceWaitIdle(&app->graphics_device);

	DebugLogI(app->log_channel, "Destroying...");
	
	E_WorldDestroy(&app->world);

	P_EngineDestroy(&app->physics_engine);

	R_SystemDestroy(&app->render_system);
	R_SceneDestroy(&app->scene);
	R_GraphDestroy(&app->graph);
	G_RingBufferDestroy(&app->frame_upload_ring_buffer, &app->graphics_device);

	AN_SystemDestroy(&app->animation_system);
	
	A_Destroy(&app->assets);
	
	AU_Shutdown(&app->audio_system);
	
	AU_BackendShutdown(app->audio_backend);
	
	G_ShaderCompilerShutdown(&app->shader_compiler);
	G_DeviceSwapchainDestroy(&app->graphics_device, &app->swapchain);
	G_DeviceDestroy(&app->graphics_device);

	S_Destroy(app->scripting_system);

	ArenaRelease(&app->frame_arena);
	ArenaRelease(&app->entity_arena);
	ArenaRelease(&app->physics_arena);
	ArenaRelease(&app->render_arena);
	ArenaRelease(&app->animation_arena);
	ArenaRelease(&app->asset_arena);
	ArenaRelease(&app->audio_arena);
	ArenaRelease(&app->graphics_arena);
	
	DebugLogI(app->log_channel, "Destroyed");

	// TODO: fuck are we remembering to release this? why wasnt it crashing lol
	//ArenaRelease(&app->bootstrap_arena);
}

static void AppLogFPS(f32 dt)
{
	static u32 index = 0;
	static f32 fps_history[1] = {0};

	const f32 fps_now = 1.f / dt;

	fps_history[index % ArraySize(fps_history)] = fps_now;
	index++;

	f32 fps_avg = 0.f;

	for (u32 i = 0; i < ArraySize(fps_history); i++)
		fps_avg += fps_history[i];
	
	fps_avg /= (f32)ArraySize(fps_history);
	
	//DebugLogT(app->log_channel, "FPS: %.2f", fps_avg);
}

__declspec(dllexport)
b32 MagpieTick(App *app_, const OS_InputState *input)
{
	if (OS_KbPressed(input, OS_KeyboardKey_Escape))
		return true;
	
	const f32 max_frame_time = 0.2f;
	const f32 elapsed = CH_TimerElapsed(&app->elapsed_timer);
	const f32 dt = CH_TimerRestart(&app->delta_timer);
	const f32 fixed_dt = 1.f / APP_TARGET_FPS;

	if (CH_TimerElapsed(&app->hot_reload_timer) >= APP_HOT_RELOAD_INTERVAL)
	{
		CH_TimerRestart(&app->hot_reload_timer);
		A_PollHotReloads(&app->assets);
	}

	A_FlushUploads(&app->assets);

	E_TickContext entity_tick_context = {0};
	entity_tick_context.dt = dt;
	entity_tick_context.elapsed = elapsed;
	entity_tick_context.input = input;

	E_WorldResolveInittingEntities(&app->world);

	E_WorldTickPreAnim(&app->world, &entity_tick_context);
	
	AN_SystemCalculateIntermediatePoses(&app->animation_system, elapsed);

	E_WorldTickPostAnim(&app->world, &entity_tick_context);

	GameTick(input, dt, elapsed);

	S_Tick(app->scripting_system, dt);
	
	f32 clamped_delta = dt;

	if (dt > max_frame_time)
	{
		DebugLogW(app->log_channel, "Had to clamp delta (was %f, clamped to %f).", dt, max_frame_time);
		clamped_delta = max_frame_time;
	}
	
	app->delta_accumulator += clamped_delta;

	while (app->delta_accumulator >= fixed_dt)
	{
		P_EngineTick(&app->physics_engine, fixed_dt);
		app->delta_accumulator -= fixed_dt;
	}

	AN_SystemFinalizePoseAndMatrixPalette(&app->animation_system);
	
	E_WorldTickPostPhysics(&app->world, &entity_tick_context);

	E_EventDispatch(&app->events, &app->world);

	E_WorldFlush(&app->world);
	
	AU_Listener listener = {0};
	listener.position = app->game.camera.position;
	listener.direction = app->game.camera.forward;

	AU_Tick(&app->audio_system, dt, listener);
	AU_BackendTick(app->audio_backend, dt, listener);

	//R_IrradianceVolumeDebug(&app->irradiance_volume);
	//R_SceneDebug(&app->scene);

	// https://gafferongames.com/post/fix_your_timestep/
	const float alpha = app->delta_accumulator / fixed_dt;

	R_CameraRecompute(&app->game.camera);
	
	R_FrameParams curr_frame = {0};
	curr_frame.arena = &app->frame_arena;
	curr_frame.frame_number = app->frame_count;
	curr_frame.dt = dt;
	curr_frame.elapsed = elapsed;
	curr_frame.scene_data = R_SceneUploadFrameData(&app->scene, &app->frame_upload_ring_buffer);
	curr_frame.camera = app->game.camera;
	
	R_FrameParams interpolated_frame = R_FrameParamsInterp(&app->prev_frame, &curr_frame, alpha);
	
	R_SystemRender(&app->render_system, &app->graph, &interpolated_frame);

	R_GraphCompile(&app->graph, &app->swapchain);
	
	{
		G_CmdBuffer cmd = G_DeviceBeginFrame(&app->graphics_device, &app->swapchain);
		R_GraphExecute(&app->graph, &app->swapchain, &cmd, &app->scene, &app->game.camera, dt, elapsed);
		R_GraphPresentToSwapchain(&app->graph, &app->swapchain, &cmd);
		G_DeviceEndFrame(&app->graphics_device, &app->swapchain, &cmd);
	}
	
	R_GraphReset(&app->graph);
	
	G_RingBufferReset(&app->frame_upload_ring_buffer);
	ArenaReset(&app->frame_arena);
	
	AppLogFPS(dt);
	
	app->frame_count++;

	app->prev_frame = curr_frame;
	
	return false;
}

__declspec(dllexport)
void MagpieHotLoad(App *app_, const OS_API *api)
{
	osapi = api;
	app = app_;
	
	/*
	 * this is a super hacky measure but there's no other
	 * way to do this that isn't super complicated.
	 * essentially when we hot reload, the function bindings
	 * associated with C functions become invalidated, and
	 * we can either
	 * a. destroy and recreate the entire system (lose all state)
	 * b. iterate through every goddamn C closure in the lua registry and every upvalue
	 * option b. is a complete nightmare i dont wanna deal with right
	 * now so in my opinion it's far more preferable to just reset the whole thing.
	 */
	
	ArenaReset(&app->scripting_arena);
	AppInitScripting();

	G_DeviceHotLoad(&app->graphics_device);
	R_SystemHotLoad(&app->render_system);
	
	GameSelect(&app->game);
}

__declspec(dllexport)
void MagpieHotUnload(App *app_)
{
	G_DeviceHotUnload(&app->graphics_device);

	// hack
	S_Destroy(app->scripting_system);
}

/*
R_Light light = {0};
light.type = R_LightType_Point;
light.position = v3(0.f, 0.f, 1.f);
light.direction = v3x(0.f);
light.colour = v3(1.f, 1.f, 1.f);
light.intensity = 5.f;
light.falloff = 1.f;
light.casts_shadows = true;
light.shadow_near = 0.1f;
light.shadow_far = 10.f;

//app->light_handle = R_SceneLightCreate(&app->scene, &light);

//A_Handle object_model_handle = A_Require(&app->assets, String8Lit("assets://models/Sponza/glTF/Sponza.gltf"),                     A_Type_Model);
A_Handle object_model_handle = A_Require(&app->assets, String8Lit("assets://models/DamagedHelmet/glTF/DamagedHelmet.gltf"),       A_Type_Model);
//A_Handle object_model_handle = A_Require(&app->assets, String8Lit("assets://models/CompareSheen/glTF/CompareSheen.gltf"),         A_Type_Model);
//A_Handle object_model_handle = A_Require(&app->assets, String8Lit("assets://models/CompareClearcoat/glTF/CompareClearcoat.gltf"), A_Type_Model);
//A_Handle object_model_handle = A_Require(&app->assets, String8Lit("assets://models/SimpleSkin/glTF/SimpleSkin.gltf"),             A_Type_Model);
//A_Handle object_model_handle = A_Require(&app->assets, String8Lit("assets://models/RiggedFigure/glTF/RiggedFigure.gltf"),         A_Type_Model);
//A_Handle object_model_handle = A_Require(&app->assets, String8Lit("assets://models/RiggedSimple/glTF/RiggedSimple.gltf"),         A_Type_Model);

ScratchArena scratch = ScratchBegin(NULL, 0);
{
	G_CmdBuffer cmd = G_DeviceSubmitImBegin(&app->graphics_device);
	R_ModelImportReceipt receipt = R_SceneImportModel(&app->scene, &cmd, scratch.arena, object_model_handle, (u32)(-1));
	G_DeviceSubmitImEnd(&app->graphics_device, &cmd);

	for (u32 i = 0; i < receipt.count; i++)
	{
		R_ModelImportEntry *entry = &receipt.entries[i];

		R_ObjectDesc desc = {0};
		desc.transform = entry->transform;
		desc.sphere_bounds = entry->sphere_bounds;
		desc.mesh = entry->mesh;
		desc.material = entry->material;

		R_SceneObjectCreate(&app->scene, &desc);
	}
}
ScratchRelease(&scratch);

app->test_sound_handle = A_Require(&app->assets, String8Lit("assets://sounds/test_sound.mp3"), A_Type_Sound);
A_Asset *test_sound_asset = A_GetNow(&app->assets, app->test_sound_handle);
app->test_sound = test_sound_asset->sound.buffer;

A_Handle test_script_handle = A_Require(&app->assets, String8Lit("assets://test.lua"), A_Type_Script);
S_Ref test_lua_script = A_GetNow(&app->assets, test_script_handle)->script.ref;
S_CallMethod(app->scripting_system, test_lua_script, String8Lit("Yay"));

E_WorldSpawn(&app->world, E_Type_Player, E_TransformIdentity());
*/

/*
	if (OS_KbPressed(input, OS_KeyboardKey_Enter))
	{
	S_FireSignal(app->scripting_system, String8Lit("test_ready"));
	//R_IrradianceVolumeBake(&app->irradiance_volume, &app->scene);
	}

	if (OS_KbPressed(input, OS_KeyboardKey_Y))
	{
	AU_PlayConfig play_config = {0};
	play_config.clip = app->test_sound;
	play_config.bus = AU_Bus_Sfx;
	play_config.volume = 1.f;
	play_config.pitch = 1.f;
	play_config.spatial = true;
	play_config.position = v3x(0.f);
	
	AU_Play(&app->audio_system, &play_config);
	}

	if (OS_KbDown(input, OS_KeyboardKey_Up  ))  app_pp_exposure += dt;
	if (OS_KbDown(input, OS_KeyboardKey_Down))  app_pp_exposure -= dt;

	R_SceneLightSetPosition(&app->scene, app->light_handle, v3(SinF(elapsed*2.f)*2.f, 0.f, 1.f));
*/

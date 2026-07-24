
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

internal S_BINDING_DEF(S_BND_WaitSeconds)
{
	f32 s = S_CtxGetArgF32(ctx, 0); 
	S_CtxYieldTime(ctx, s);
}

internal S_BINDING_DEF(S_BND_WaitSignal)
{
	String8 sig = S_CtxGetArgStr(ctx, 0);
	S_CtxYieldSignal(ctx, sig);
}

internal S_BINDING_DEF(S_BND_DebugLog)
{
	String8 msg = S_CtxGetArgStr(ctx, 0);
	
	DebugLogD(app->log_channel, "%.*s", String8VArg(msg));
}

internal void AppInitScripting(void)
{
	app->scripting_system = S_AllocAndSelect(&app->scripting_arena, app->scripting_log_channel);

	S_BindGlobal(String8Lit("wait_seconds"), S_BND_WaitSeconds);
	S_BindGlobal(String8Lit("wait_signal"), S_BND_WaitSignal);
	S_BindGlobal(String8Lit("debug_log"), S_BND_DebugLog);
}

internal void AppInit_(void)
{
	AppInitScripting();
	
	G_FeatureRequest features[] = {
		{ G_FeatureType_RayTracing, G_FeatureTier_Optional }
	};
	
	G_InitAndSelect(&app->graphics_device, &app->graphics_arena, app->graphics_log_channel, features, ArraySize(features));
	app->swapchain = G_SwapchainCreate();
	G_ShaderCompilerInitAndSelect(&app->shader_compiler, osapi->LogChannelOpenFrom(app->graphics_log_channel, String8Lit("SLANG")));

	//app->audio_backend = AU_BackendAllocAndSelect(&app->audio_arena, osapi->LogChannelOpenFrom(app->audio_log_channel, String8Lit("MINI")));
	//AU_InitAndSelect(&app->audio_system, &app->audio_arena, app->audio_log_channel);
	
	A_InitAndSelect(&app->assets, &app->asset_arena, app->asset_log_channel);
	
	//A_Mount(String8Lit("engine://shaders"), String8Lit("src/render/shaders"));
	A_Mount(String8Lit("assets://"), String8Lit("res"));

	AN_SystemInitAndSelect(&app->animation_system, app->animation_log_channel);
	
	R_GraphInit(&app->graph, &app->render_arena, osapi->LogChannelOpenFrom(app->render_log_channel, String8Lit("GRAPH")));
	R_SceneInit(&app->scene, &app->render_arena, osapi->LogChannelOpenFrom(app->render_log_channel, String8Lit("SCENE")));
	R_SystemInitAndSelect(&app->render_system, &app->render_arena, osapi->LogChannelOpenFrom(app->render_log_channel, String8Lit("SYSTEM")));
	R_ModelCatalogueInit(&app->model_catalogue, &app->render_arena);
	R_ModelCatalogueEquipScene(&app->model_catalogue, &app->scene);
	
	P_EngineInitAndSelect(&app->physics_engine, &app->physics_arena, app->physics_log_channel);
	
	E_WorldInit(&app->world, &app->entity_arena, osapi->LogChannelOpenFrom(app->entity_log_channel, String8Lit("WORLD")));
	E_EventQueueInit(&app->events, osapi->LogChannelOpenFrom(app->entity_log_channel, String8Lit("EVENT")));

	GameSelect(&app->game);
	GameInit(&app->world);

	CH_TimerStart(&app->elapsed_timer);
	CH_TimerStart(&app->delta_timer);
	CH_TimerStart(&app->hot_reload_timer);
}

App *MagpieInit(const OS_API *osapi_)
{
	osapi = osapi_;
	
	app = osapi->HeapAlloc(sizeof(App));
	app->bootstrap_memory = app; // funky
	
	app->scripting_arena = ArenaAlloc(Gigabytes(1));
	app->graphics_arena  = ArenaAlloc(Gigabytes(3));
	app->audio_arena     = ArenaAlloc(Gigabytes(1));
	app->asset_arena     = ArenaAlloc(Gigabytes(3));
	app->animation_arena = ArenaAlloc(Gigabytes(1));
	app->render_arena    = ArenaAlloc(Gigabytes(2));
	app->physics_arena   = ArenaAlloc(Gigabytes(1));
	app->entity_arena    = ArenaAlloc(Gigabytes(1));
	app->frame_arena     = ArenaAlloc(Gigabytes(1));
	
	app->log_channel           = osapi->LogChannelOpen(String8Lit("APP"));
	app->scripting_log_channel = osapi->LogChannelOpen(String8Lit("SCRIPT"));
	app->graphics_log_channel  = osapi->LogChannelOpen(String8Lit("GRAPHICS"));
	app->audio_log_channel     = osapi->LogChannelOpen(String8Lit("AUDIO"));
	app->asset_log_channel     = osapi->LogChannelOpen(String8Lit("ASSETS"));
	app->animation_log_channel = osapi->LogChannelOpen(String8Lit("ANIMATION"));
	app->render_log_channel    = osapi->LogChannelOpen(String8Lit("RENDER"));
	app->physics_log_channel   = osapi->LogChannelOpen(String8Lit("PHYSICS"));
	app->entity_log_channel    = osapi->LogChannelOpen(String8Lit("ENTITY"));
	
	AppInit_();
	
	{
		R_ModelInstanceCreateFromPath(&app->model_catalogue,
									  String8Lit("assets://models/Sponza/glTF/Sponza.gltf"),
									  M4Scale(v3x(5.f)));
	}

	{
		R_Light light = {0};
		light.type = R_LightType_Point;
		light.position = v3(0.f, 0.f, 1.f);
		light.direction = v3x(0.f);
		light.colour = v3(1.f, 1.f, 1.f);
		light.intensity = 5.f;
		light.falloff = 0.1f;
		light.casts_shadows = false;
		light.shadow_near = 0.1f;
		light.shadow_far = 20.f;

		R_EntityHandle entity = R_SceneEntityCreate(&app->scene, R_EntityType_Light);
		
		R_SceneSetLightParam(&app->scene, entity, light);
	}
	
	{
		R_Light light = {0};
		light.type = R_LightType_Point;
		light.position = v3(5.f, 4.f, 1.f);
		light.direction = v3x(0.f);
		light.colour = v3(0.f, 1.f, 1.f);
		light.intensity = 5.f;
		light.falloff = 0.1f;
		light.casts_shadows = false;
		light.shadow_near = 0.1f;
		light.shadow_far = 20.f;

		R_EntityHandle entity = R_SceneEntityCreate(&app->scene, R_EntityType_Light);
		
		R_SceneSetLightParam(&app->scene, entity, light);
	}
	
	DebugLogI(app->log_channel, "Initialized.");
	
	return app;
}

void MagpieDestroy(App *app_)
{
	G_WaitIdle();

	DebugLogI(app->log_channel, "Destroying...");
	
	E_WorldDestroy(&app->world);

	P_EngineDestroy();

	R_ModelCatalogueDestroy(&app->model_catalogue);
	R_SystemDestroy();
	R_SceneDestroy(&app->scene);
	R_GraphDestroy(&app->graph);

	AN_SystemDestroy();
	
	A_Destroy();
	
	//AU_Shutdown();
	//AU_BackendShutdown();
	
	G_ShaderCompilerShutdown();
	G_SwapchainDestroy(&app->swapchain);
	G_Destroy();

	S_Destroy();

	ArenaRelease(&app->frame_arena);
	ArenaRelease(&app->entity_arena);
	ArenaRelease(&app->physics_arena);
	ArenaRelease(&app->render_arena);
	ArenaRelease(&app->animation_arena);
	ArenaRelease(&app->asset_arena);
	ArenaRelease(&app->audio_arena);
	ArenaRelease(&app->scripting_arena);
	ArenaRelease(&app->graphics_arena);
	
	DebugLogI(app->log_channel, "Destroyed.");

	osapi->HeapFree(app->bootstrap_memory);
}

internal void AppLogFPS(f32 dt)
{
	static u32 index = 0;
	static f32 fps_history[16] = {0};

	const f32 fps_now = 1.f / dt;

	fps_history[index % ArraySize(fps_history)] = fps_now;
	index++;

	f32 fps_avg = 0.f;

	for (u32 i = 0; i < ArraySize(fps_history); i++)
		fps_avg += fps_history[i];
	
	fps_avg /= (f32)ArraySize(fps_history);
	
	DebugLogT(app->log_channel, "FPS: %.2f", fps_avg);
}

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
		A_PollHotReloads();
	}

	A_FlushUploads();

	E_TickContext entity_tick_context = {0};
	entity_tick_context.dt = dt;
	entity_tick_context.elapsed = elapsed;
	entity_tick_context.input = input;

	E_WorldResolveInittingEntities(&app->world);

	E_WorldTickPreAnim(&app->world, &entity_tick_context);
	
	AN_SystemCalculateIntermediatePoses(elapsed);

	E_WorldTickPostAnim(&app->world, &entity_tick_context);

	S_Tick(dt);
	
	f32 clamped_delta = dt;

	if (dt > max_frame_time)
	{
		DebugLogW(app->log_channel, "Had to clamp delta (was %f, clamped to %f).", dt, max_frame_time);
		clamped_delta = max_frame_time;
	}
	
	app->delta_accumulator += clamped_delta;

	while (app->delta_accumulator >= fixed_dt)
	{
		P_EngineTick(fixed_dt);
		GameTick(input, fixed_dt, elapsed);
		app->delta_accumulator -= fixed_dt;
	}

	AN_SystemFinalizePoseAndMatrixPalette();
	
	E_WorldTickPostPhysics(&app->world, &entity_tick_context);

	E_EventDispatch(&app->events, &app->world);

	E_WorldFlush(&app->world);

	//AU_Listener listener = {0};
	//listener.position = app->game.camera.position;
	//listener.direction = app->game.camera.forward;

	//AU_Tick(dt, listener);
	//AU_BackendTick(dt, listener);

	//R_IrradianceVolumeDebug(&app->irradiance_volume);
	//R_SceneDebug(&app->scene);

	// https://gafferongames.com/post/fix_your_timestep/
	const float alpha = app->delta_accumulator / fixed_dt;

	R_CameraRecompute(&app->game.camera);
	
	R_SceneFlushIfDirty(&app->scene);
	
	R_FrameParams frame_params = R_FrameParamsBuild(&app->frame_arena,
													&app->render_system,
													app->frame_number, dt, elapsed,
													&app->scene, &app->game.camera);

	if (app->frame_number == 0)
	{
		R_SystemGenerateLookupsAndMaps(&app->graph, &app->frame_arena, &frame_params);		

		// genuinely fuck this
		// BRDF LUT and IBL shouldnt be generated as part of the RENDER GRAPH FUCK
		frame_params = R_FrameParamsBuild(&app->frame_arena,
										  &app->render_system,
										  app->frame_number, dt, elapsed,
										  &app->scene, &app->game.camera);
	}

	// render debug sphere bounds on objects for culling debug.
	/*
	{
		R_EntityIterator iterator = R_EntityIteratorInit(&app->scene);
		R_Entity *entity = NULL;
	
		while ((entity = R_EntityIteratorNext(&iterator, R_EntityType_Object)))
		{
			R_Object *object = &entity->object;
			
			v4 local_sphere_bounds = object->local_sphere_bounds;

			v3 local_centre = v3(local_sphere_bounds.x,
								 local_sphere_bounds.y,
								 local_sphere_bounds.z);
	
			v3 local_edge = v3(local_sphere_bounds.x + local_sphere_bounds.w,
							   local_sphere_bounds.y,
							   local_sphere_bounds.z);
 
			v3 world_centre = M4MulV3Point(object->transform, local_centre);
			v3 world_edge = M4MulV3Point(object->transform, local_edge);
			
			v3 dx = V3Sub(world_edge, world_centre);
			f32 world_radius = V3Length(dx);

			R_DebugPushSphere(world_centre,
							  world_radius,
							  v4(1.f, 1.f, 1.f, 1.f),
							  0.f, true);
		}
	}
	*/
	
	R_SystemRender(&app->graph, &frame_params);
	
	R_GraphCompile(&app->graph, &app->swapchain);
	
	{ // --- [[ THE GRAPHICS ZONE ooOooOooOOOooOOo !! ]] ---
		G_CmdBuffer cmd = G_BeginFrame(&app->swapchain);
		R_GraphExecute(&app->graph, &app->swapchain, &cmd);
		R_GraphPresentToSwapchain(&app->graph, &app->swapchain, &cmd);
		G_EndFrame(&app->swapchain, &cmd);
	} // ---
	
	R_GraphReset(&app->graph);
	
	ArenaReset(&app->frame_arena);
	
	AppLogFPS(dt);

	app->frame_number++;
	
	return false;
}

void MagpieHotLoad(App *app_, const OS_API *osapi_)
{
	osapi = osapi_;
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

	G_SelectContext(&app->graphics_device);
	G_ShaderCompilerSelectContext(&app->shader_compiler);
	G_HotLoad();

	//AU_BackendSelectContext(app->audio_backend);
	//AU_SelectContext(&app->audio_system);

	A_SelectContext(&app->assets);

	AN_SystemSelectContext(&app->animation_system);

	R_SystemSelectContext(&app->render_system);
	
	P_EngineSelectContext(&app->physics_engine);

	GameSelect(&app->game);
}

void MagpieHotUnload(App *app_)
{
	G_HotUnload();

	// hack
	S_Destroy();
}

/*
  app->test_sound_handle = A_Require(&app->assets, String8Lit("assets://sounds/test_sound.mp3"));
  A_Asset *test_sound_asset = A_GetNow(&app->assets, app->test_sound_handle);
  app->test_sound = test_sound_asset->sound.buffer;

  A_Handle test_script_handle = A_Require(&app->assets, String8Lit("assets://test.lua"), A_Type_Script);
  S_Ref test_lua_script = A_GetNow(&app->assets, test_script_handle)->script.ref;
  S_CallMethod(app->scripting_system, test_lua_script, String8Lit("Yay"));
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

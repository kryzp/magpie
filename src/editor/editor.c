
internal void
EditorInit(Editor *editor, Arena *arena, LOG_Channel log_channel)
{
	editor->arena = arena;
	editor->log_channel = log_channel;
	
	GM_StackInit(&editor->game_mode_stack);

	editor->camera = R_CameraPerspective(v3x(0.f), v3(0.f, 1.f, 0.f), 90.f, 1280.f / 720.f, .1f, 100.f);

	CameraDriverConfig camera_driver_cfg = {0};
	camera_driver_cfg.mode = CameraDriverMode_Unrestricted;
	editor->camera_driver = CameraDriverInit(&camera_driver_cfg);
	
	DebugLogI(editor->log_channel, "Initialized.");
}

internal void
EditorDestroy(Editor *editor)
{
	DebugLogI(editor->log_channel, "Destroyed.");
}

internal void
EditorTick(Editor *editor, const I_State *input, f32 dt, f32 elapsed)
{
	GM_StackTick(&editor->game_mode_stack, editor, dt, input);
	
	CameraDriverDrive(&editor->camera_driver, &editor->camera, input, dt);
}

internal void
EditorHotLoad(Editor *editor)
{
}

internal void
EditorHotUnload(Editor *editor)
{
}

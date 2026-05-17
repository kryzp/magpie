#ifndef EDITOR_H
#define EDITOR_H

typedef struct Editor Editor;
struct Editor
{
	Arena *arena;
	
	LOG_Channel log_channel;
	
	GM_Stack game_mode_stack;

	R_Camera camera;
	
	CameraDriver camera_driver;
	b32          camera_driver_active;
};

internal void EditorInit      (Editor *editor, Arena *arena, LOG_Channel log_channel);
internal void EditorDestroy   (Editor *editor);
internal void EditorTick      (Editor *editor, const OS_InputState *input, f32 dt, f32 elapsed);
internal void EditorHotLoad   (Editor *editor);
internal void EditorHotUnload (Editor *editor);

#endif // EDITOR_H

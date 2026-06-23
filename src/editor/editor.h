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

static void EditorInit      (Editor *editor, Arena *arena, LOG_Channel log_channel);
static void EditorDestroy   (Editor *editor);
static void EditorTick      (Editor *editor, const OS_InputState *input, f32 dt, f32 elapsed);
static void EditorHotLoad   (Editor *editor);
static void EditorHotUnload (Editor *editor);

#endif // EDITOR_H

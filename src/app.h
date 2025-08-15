
typedef struct App
{
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scratch_arenas[2];
	
	GraphicsDevice graphics_device;
	Renderer renderer;
}
App;

global App *app = 0;
global Platform *platform = 0;
global GraphicsDevice *graphics_device = 0;

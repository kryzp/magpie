
typedef struct Core
{
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scratch_arenas[2];
	
	GraphicsDevice graphics_device;
	Renderer renderer;
}
Core;

global Core *core = 0;
global Platform *platform = 0;
global GraphicsDevice *graphics_device = 0;

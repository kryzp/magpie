
typedef struct Core
{
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scratch_arenas[2];
	
	u64 starting_ticks;
	
	GraphicsDevice graphics_device;
	VertexFormats vertex_formats;
	Renderer renderer;
	
	Assets assets;
	
	Camera main_camera;
	
	Model damaged_helmet_model;
}
Core;

global Core *core = 0;
global Platform *platform = 0;
global GraphicsDevice *graphics_device = 0;
global VertexFormats *vertex_formats = 0;

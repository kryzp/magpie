
typedef struct App
{
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scratch_arenas[2];
	
	Renderer renderer;
}
App;

global App *app = 0;

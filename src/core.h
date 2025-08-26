
typedef struct Core
{
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scratch_arenas[2];
	
	u64 starting_ticks;
	
	GraphicsDevice graphics_device;
	
	Renderer renderer;
	RenderContext render_context;
	RenderGraph render_graph;
	
	VertexFormats vertex_formats;
	
	Assets assets;
	
	Camera main_camera;
	Scene scene;
	
	u32 damaged_helmet_object;
	Image environment_hdr_texture;
}
Core;

global Core *core = 0;
global Platform *platform = 0;
global GraphicsDevice *graphics_device = 0;
global VertexFormats *vertex_formats = 0;

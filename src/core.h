
typedef struct Core {
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scratch_arenas[2];

	u64 starting_ticks;

	GraphicsDevice graphics_device;
	RenderState render_state;
	RenderGraph render_graph;
	Renderer renderer;

	VertexFormats vertex_formats;
	Shaders shaders;

	Assets assets;

	Camera main_camera;
	Scene scene;

	// ---

	u32 damaged_helmet_objects[5];

	// ---

	Image brdf_lut_image;

	// TODO: Skybox and environment probes should be local to a scene, not just part of the render context?
	Image skybox_cubemap;
	EnvironmentProbe environment_probe;

	GPUBuffer cubemap_capture_transforms;

	Sampler linear_sampler;

	Mesh skybox_mesh;
	Mesh light_sphere_mesh;
} Core;

global Core *core = 0;
global Platform *platform = 0;
global GraphicsDevice *graphics_device = 0;
global VertexFormats *vertex_formats = 0;
global Shaders *shaders = 0;

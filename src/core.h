
// NOTE(kp): Per-frame data like shader buffers.
typedef struct CoreFrameData {
	GPUBuffer frame_data_buffer;
	GPUBuffer object_buffer;
	GPUBuffer light_buffer;
	GPUBuffer indirect_buffer;
} CoreFrameData;

typedef struct Core {
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scratch_arenas[2];

	u64 starting_ticks;

	GraphicsDevice graphics_device;

	Renderer renderer;
	RenderContext render_context;
	RenderGraph render_graph;

	VertexFormats vertex_formats;
	Shaders shaders;

	Assets assets;

	Camera main_camera;
	Scene scene;

	// ---

	u32 damaged_helmet_objects[5];

	// ---

	Image brdf_lut_image;

	// TODO(kp): Skybox and environment probes should be local to a scene, not just part of the render context?
	Image skybox_cubemap;
	EnvironmentProbe environment_probe;

	GPUBuffer cubemap_capture_transforms;

	CoreFrameData per_frame_data[FRAMES_IN_FLIGHT];

	Sampler linear_sampler;

	Mesh skybox_mesh;
	Mesh light_sphere_mesh;
} Core;

internal CoreFrameData *CoreGetCurrentFrameData();

global Core *core = 0;
global Platform *platform = 0;
global GraphicsDevice *graphics_device = 0;
global VertexFormats *vertex_formats = 0;
global Shaders *shaders = 0;

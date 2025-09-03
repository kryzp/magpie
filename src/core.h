
typedef struct CoreFrameData {
	GPUBuffer frame_data_buffer;
	GPUBuffer object_buffer;
	GPUBuffer light_buffer;
	GPUBuffer indirect_buffer;
} CoreFrameData;

typedef struct Core {
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scene_arena;
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

	CoreFrameData per_frame_data[FRAMES_IN_FLIGHT];
	GPUBuffer material_buffer;
	MeshPass mesh_pass;
	
	// ---

	u32 damaged_helmet_objects[5];
	u32 light;
	
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

internal CoreFrameData *CoreCurrentFrame();

global Core *core = NULL;
global Platform *platform = NULL;
global GraphicsDevice *graphics_device = NULL;
global VertexFormats *vertex_formats = NULL;
global Shaders *shaders = NULL;

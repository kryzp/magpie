
typedef struct CoreFrameData {
	GPUBuffer frame_data_buffer;
	GPUBuffer object_buffer;
	GPUBuffer light_buffer;
} CoreFrameData;

typedef struct Core {
	MemoryArena permanent_arena;
	MemoryArena frame_arena;
	MemoryArena scene_arena;
	MemoryArena scratch_arenas[2];
	
	u64 starting_ticks;
	
	Timer delta_timer;
	f32 delta_accumulator;

	GraphicsDevice graphics_device;
	RenderState render_state;
	RenderGraph render_graph;

	VertexFormats vertex_formats;
	Shaders shaders;
	
	Assets assets;

	Camera main_camera;
	CameraDriver main_camera_driver;
	
	Scene scene;

	// ---

	GBuffer gbuffer;

	CoreFrameData per_frame_data[FRAMES_IN_FLIGHT];

	GPUBuffer material_buffer;

	Image brdf_lut_image;
	GPUBuffer cubemap_capture_transforms;
	Sampler linear_sampler;
	Mesh skybox_mesh;
	Mesh light_sphere_mesh;

	// TODO: Skybox and environment probes should be local to a scene, not just part of the core.
	Image skybox_cubemap;
	EnvironmentProbe environment_probe;

	GPUBuffer compacted_instance_buffer; // <u32>
	GPUBuffer instance_buffer;           // <GPU_Instance>
	GPUBuffer draw_indirect_buffer;      // <GPU_IndirectObject>
	GPUBuffer clear_indirect_buffer;     // <GPU_IndirectObject>

	b32 instance_buffer_dirty;
	b32 indirect_buffer_dirty;
	
	// ---

	u32 damaged_helmet_objects[16][16];
	u32 light;
} Core;

global Core *core = NULL;
global Platform *platform = NULL;
global GraphicsDevice *graphics_device = NULL;
global VertexFormats *vertex_formats = NULL;
global Shaders *shaders = NULL;

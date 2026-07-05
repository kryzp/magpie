#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

typedef struct R_System R_System;
struct R_System
{
	Arena *arena;
	G_Device *device;
	A_Assets *assets;	
	LOG_Channel log_channel;
	
	G_RingBuffer frame_upload_ring_buffer;
	
	G_BufferKey cubemap_capture_transform_buffer;
	
	G_SamplerKey linear_sampler;
	G_SamplerKey nearest_sampler;

	//R_ShadowState shadow_renderer;
	R_DebugRenderer debug_renderer;
	//R_IrradianceVolume irradiance_volume;
	
	R_Mesh skybox_mesh;
	
	G_TextureKey brdf_lut;
	G_TextureKey environment_cubemap;
	G_TextureKey irradiance_cubemap;
	G_TextureKey prefilter_cubemap;

	u32 frame_count;
};

static void R_SystemCreateSkyboxMesh(R_System *s);
static void R_SystemInit(R_System *s, Arena *arena, G_Device *device, A_Assets *assets, LOG_Channel log_channel);
static void R_SystemDestroy(R_System *s);
static void R_SystemGenerateLookupsAndMaps(R_System *s, R_Graph *g, Arena *arena);
static void R_SystemRender(R_System *s, R_Graph *graph, const R_FrameParams *frame_params);
static void R_SystemHotLoad(R_System *s);

#endif // RENDER_SYSTEM_H

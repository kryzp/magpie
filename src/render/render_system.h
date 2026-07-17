#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

typedef struct R_System R_System;
struct R_System
{
	Arena *arena;
	LOG_Channel log_channel;
	
	G_RingBuffer frame_upload_ring_buffer;
	
	R_ShadowState shadow_render_state;
	R_DebugRenderer debug_renderer;
	//R_IrradianceVolume irradiance_volume;

	G_TextureKey brdf_lut;
	G_TextureKey environment_cubemap;
	G_TextureKey irradiance_cubemap;
	G_TextureKey prefilter_cubemap;

	R_Mesh skybox_mesh;

	G_BufferKey cubemap_capture_transform_buffer;
	
	struct
	{
		A_Handle debug_line_handle;
		A_Handle forward_handle;
		A_Handle shadow_handle;
		A_Handle cull_frustum_handle;
		A_Handle cull_sphere_handle;
		A_Handle skybox_handle;
		A_Handle tonemapping_handle;
		A_Handle brdf_lut_generation_handle;
		A_Handle hdr_to_cubemap_handle;
		A_Handle irradiance_cubemap_gen_handle;
		A_Handle prefilter_cubemap_gen_handle;
	}
	shaders;

	struct
	{
		G_SamplerKey linear;
		G_SamplerKey nearest;
	}
	samplers;
};

static void R_SystemInitAndSelect(R_System *system, Arena *arena, LOG_Channel log_channel);
static void R_SystemDestroy(void);
static void R_SystemSelectContext(R_System *system);
static void R_SystemGenerateLookupsAndMaps(R_Graph *graph, Arena *pass_arena, const R_FrameParams *frame_params);
static void R_SystemRender(R_Graph *graph, const R_FrameParams *frame_params);
static void R_SystemHotLoad(void);

#endif // RENDER_SYSTEM_H

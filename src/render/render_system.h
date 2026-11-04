#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#define R_SYSTEM_PREFILTER_MIPS 5

typedef struct R_System R_System;
struct R_System
{
	Arena *arena;
	LOG_Channel log_channel;

	G_RingBuffer upload_ring;
	G_RingBuffer upload_slices[G_FRAMES_IN_FLIGHT];

	A_Handle environment_hdr_handle;
	
	R_ShadowState shadow_render_state;
	R_DebugRenderer debug_renderer;
	//R_IrradianceVolume irradiance_volume;

	G_ResourceKey brdf_lut;
	G_ResourceKey environment_cubemap;
	G_ResourceKey irradiance_cubemap;
	G_ResourceKey prefilter_cubemap;

	R_Mesh skybox_mesh;

	G_ResourceKey cubemap_capture_transform_buffer;
	
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
		G_ResourceKey linear;
		G_ResourceKey nearest;
	}
	samplers;
};

internal void R_SystemInitAndSelect(R_System *system, Arena *arena, LOG_Channel log_channel);
internal void R_SystemDestroy(void);
internal void R_SystemSelectContext(R_System *system);
internal G_RingBuffer *R_SystemBeginFrameUploads(R_System *system, u32 frame_slot_index);
internal void R_SystemBakeLookupsAndMaps(R_Graph *graph, const G_Swapchain *swapchain);
internal void R_SystemGenerateLookupsAndMaps(R_Graph *graph, const R_FrameParams *frame_params);
internal void R_SystemRender(R_Graph *graph, const R_FrameParams *frame_params);
internal void R_SystemHotLoad(void);

#endif // RENDER_SYSTEM_H

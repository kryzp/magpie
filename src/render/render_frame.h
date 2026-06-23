#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

typedef struct R_FrameParams R_FrameParams;
struct R_FrameParams
{
	Arena *arena;
	u64 frame_number;

	f32 dt;
	f32 elapsed;

	R_SceneFrameData scene_data; // todo this has to be interpolated

	R_Camera camera; // todo this has to be interpolated
};

static R_FrameParams R_FrameParamsInterp(const R_FrameParams *prev,
										   const R_FrameParams *curr,
										   f32 alpha);

#endif // RENDER_FRAME_H

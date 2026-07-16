#ifndef RENDER_PASS_POSTPROCESSING_H
#define RENDER_PASS_POSTPROCESSING_H

typedef struct R_PostProcessingPassData R_PostProcessingPassData;
struct R_PostProcessingPassData
{
	const R_FrameParams *frame_params;
	f32 exposure;
	R_GraphTexHandle input;
	R_GraphTexHandle output;
};

static R_PASS_RECORD_DEF(R_PostProcessingPassFn);
	
#endif // RENDER_PASS_POSTPROCESSING_H

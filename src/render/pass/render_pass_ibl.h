#ifndef RENDER_PASS_IBL_H
#define RENDER_PASS_IBL_H

typedef struct R_BRDFLutPassData R_BRDFLutPassData;
struct R_BRDFLutPassData
{
	const R_FrameParams *frame_params;
};

typedef struct R_IBLPassIrradianceData R_IBLPassIrradianceData;
struct R_IBLPassIrradianceData
{
	const R_FrameParams *frame_params;
	G_TextureViewKey env_view;
};

typedef struct R_IBLPassPrefilterData R_IBLPassPrefilterData;
struct R_IBLPassPrefilterData
{
	const R_FrameParams *frame_params;
	G_TextureViewKey env_view;
	f32 roughness;
};

static R_PASS_RECORD_DEF(R_BRDFLutPassFn);
static R_PASS_RECORD_DEF(R_IBLPassIrradianceFn);
static R_PASS_RECORD_DEF(R_IBLPassPrefilterFn);

#endif // RENDER_PASS_IBL_H

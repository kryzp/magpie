#ifndef RENDER_PASS_ENVIRONMENT_MAP_H
#define RENDER_PASS_ENVIRONMENT_MAP_H

typedef struct R_HdrToEnvPassData R_HdrToEnvPassData;
struct R_HdrToEnvPassData
{
	const R_FrameParams *frame_params;
	G_TextureViewKey hdr_view;
};

static R_PASS_RECORD_DEF(R_HdrToEnvPassFn);

#endif // RENDER_PASS_ENVIRONMENT_MAP_H

#ifndef RENDER_PASS_SKYBOX_H
#define RENDER_PASS_SKYBOX_H

typedef struct R_SkyboxPassData R_SkyboxPassData;
struct R_SkyboxPassData
{
	const R_FrameParams *frame_params;
	G_TextureViewKey cubemap;
};

static R_PASS_RECORD_DEF(R_SkyboxPassFn);
	
#endif // RENDER_PASS_SKYBOX_H

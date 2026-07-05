#ifndef RENDER_PASS_SKYBOX_H
#define RENDER_PASS_SKYBOX_H

typedef struct R_SkyboxPassData R_SkyboxPassData;
struct R_SkyboxPassData
{
	G_ShaderKey shader;
	G_TextureViewKey cubemap;
	const R_Mesh *skybox_mesh;
	const R_FrameParams *frame_params;
};

static R_PASS_RECORD_DEF(R_SkyboxPassFn);
	
#endif // RENDER_PASS_SKYBOX_H

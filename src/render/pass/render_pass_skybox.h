#ifndef RENDER_PASS_SKYBOX_H
#define RENDER_PASS_SKYBOX_H

typedef struct R_SkyboxPassData R_SkyboxPassData;
struct R_SkyboxPassData
{
	G_ShaderKey shader;
	G_TextureViewKey cubemap;
	G_SamplerKey sampler;
	G_BufferKey frame_data_buffer;
	const R_Mesh *skybox_mesh;
};

static R_PASS_RECORD_DEF(R_SkyboxPassFn);
	
#endif // RENDER_PASS_SKYBOX_H

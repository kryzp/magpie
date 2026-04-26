#ifndef RENDER_PASS_SKYBOX_H
#define RENDER_PASS_SKYBOX_H

typedef struct R_SkyboxPassData R_SkyboxPassData;
struct R_SkyboxPassData
{
	GFX_ShaderKey shader;
	GFX_TextureViewKey cubemap;
	GFX_SamplerKey sampler;
	GFX_BufferKey frame_data_buffer;
	const R_Mesh *skybox_mesh;
};

R_PASS_RECORD_DEF(R_SkyboxPassFn);
	
#endif // RENDER_PASS_SKYBOX_H

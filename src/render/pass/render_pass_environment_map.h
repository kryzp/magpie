#ifndef RENDER_PASS_ENVIRONMENT_MAP_H
#define RENDER_PASS_ENVIRONMENT_MAP_H

typedef struct R_HdrToEnvPassData R_HdrToEnvPassData;
struct R_HdrToEnvPassData
{
	G_ShaderKey shader;
	G_SamplerKey sampler;
	G_TextureViewKey hdr_view;
	G_BufferKey capture_transforms;
	const R_Mesh *skybox_mesh;
};

static R_PASS_RECORD_DEF(R_HdrToEnvPassFn);

#endif // RENDER_PASS_ENVIRONMENT_MAP_H

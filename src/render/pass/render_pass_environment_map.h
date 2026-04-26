#ifndef RENDER_PASS_ENVIRONMENT_MAP_H
#define RENDER_PASS_ENVIRONMENT_MAP_H

typedef struct R_HdrToEnvPassData R_HdrToEnvPassData;
struct R_HdrToEnvPassData
{
	GFX_ShaderKey       shader;
	GFX_SamplerKey      sampler;
	GFX_TextureViewKey  hdr_view;
	GFX_BufferKey       capture_transforms;
	const R_Mesh       *skybox_mesh;
};

R_PASS_RECORD_DEF(R_HdrToEnvPass);

#endif // RENDER_PASS_ENVIRONMENT_MAP_H

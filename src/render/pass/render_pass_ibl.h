#ifndef RENDER_PASS_IBL_H
#define RENDER_PASS_IBL_H

typedef struct R_BRDFLutPassData R_BRDFLutPassData;
struct R_BRDFLutPassData
{
	G_ShaderKey shader;
};

static R_PASS_RECORD_DEF(R_BRDFLutPassFn);

typedef struct R_IBLPassIrradianceData R_IBLPassIrradianceData;
struct R_IBLPassIrradianceData
{
	G_ShaderKey shader;
	G_SamplerKey sampler;
	G_TextureViewKey env_view;
	G_BufferKey capture_transforms;
	const R_Mesh *skybox_mesh;
};

static R_PASS_RECORD_DEF(R_IBLPassIrradianceFn);

typedef struct R_IBLPassPrefilterData R_IBLPassPrefilterData;
struct R_IBLPassPrefilterData
{
	G_ShaderKey shader;
	G_SamplerKey sampler;
	G_TextureViewKey env_view;
	G_BufferKey capture_transforms;
	const R_Mesh *skybox_mesh;
	f32 roughness;
};

static R_PASS_RECORD_DEF(R_IBLPassPrefilterFn);

#endif // RENDER_PASS_IBL_H

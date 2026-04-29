#ifndef RENDER_PASS_IBL_H
#define RENDER_PASS_IBL_H

/* for reference
   
typedef struct R_PassContext R_PassContext;
struct R_PassContext
{
	R_Graph *graph;
	
	GFX_Device *device;	
	GFX_CmdBuffer *cmd;

	const R_Scene *scene;
	const R_Camera *camera;
	
	f32 delta_time;
	f32 elapsed_time;

	const void *user_data;
};

*/

typedef struct R_IBLPassIrradianceData R_IBLPassIrradianceData;
struct R_IBLPassIrradianceData
{
	GFX_ShaderKey       shader;
	GFX_SamplerKey      sampler;
	GFX_TextureViewKey  env_view;
	GFX_BufferKey       capture_transforms;
	const R_Mesh       *skybox_mesh;
};

R_PASS_RECORD_DEF(R_IBLPassIrradianceFn);

// ---

typedef struct R_IBLPassPrefilterData R_IBLPassPrefilterData;
struct R_IBLPassPrefilterData
{
	GFX_ShaderKey       shader;
	GFX_SamplerKey      sampler;
	GFX_TextureViewKey  env_view;
	GFX_BufferKey       capture_transforms;
	const R_Mesh       *skybox_mesh;
	f32                 roughness;
};

R_PASS_RECORD_DEF(R_IBLPassPrefilterFn);

#endif // RENDER_PASS_IBL_H

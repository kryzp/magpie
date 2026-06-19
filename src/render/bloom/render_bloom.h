#ifndef RENDER_BLOOM_H
#define RENDER_BLOOM_H

// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#define R_BLOOM_MIP_CHAIN_LENGTH 5

typedef struct R_BloomRenderer R_BloomRenderer;
struct R_BloomRenderer
{
	A_Registry *assets;
	
	R_GraphTexHandle mip_chain[R_BLOOM_MIP_CHAIN_LENGTH];

	A_Handle upsample_shader_handle;
	A_Handle downsample_shader_handle;
};

internal void R_BloomRendererInit(R_BloomRenderer *renderer, A_Registry *assets);
internal void R_BloomRendererDestroy(R_BloomRenderer *renderer);

internal void R_BloomRender(R_BloomRenderer *renderer, R_Graph *graph);

#endif // RENDER_BLOOM_H

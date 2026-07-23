#ifndef RENDER_BLOOM_H
#define RENDER_BLOOM_H

// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#define R_BLOOM_MIP_CHAIN_LENGTH 5

typedef struct R_BloomState R_BloomState;
struct R_BloomState
{
	R_GraphTexHandle mip_chain[R_BLOOM_MIP_CHAIN_LENGTH];
};

internal void R_BloomRender(R_BloomState *state, R_Graph *graph);

#endif // RENDER_BLOOM_H

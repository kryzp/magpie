#ifndef RENDER_PASS_IBL_H
#define RENDER_PASS_IBL_H

internal void R_IBLPassRenderIrradiance(R_Graph *graph,
										Arena *frame_arena,
										GFX_TextureKey out,
										GFX_TextureKey environment_map,
										const R_Mesh *skybox,
										GFX_BufferKey capture_transforms);

internal void R_IBLPassRenderPrefilter(R_Graph *graph,
									   Arena *frame_arena,
									   GFX_TextureKey out,
									   GFX_TextureKey environment_map,
									   const R_Mesh *skybox,
									   GFX_BufferKey capture_transforms);

#endif // RENDER_PASS_IBL_H

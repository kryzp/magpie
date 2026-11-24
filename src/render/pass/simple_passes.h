#ifndef RENDER_PASS_SIMPLE_PASSES_H
#define RENDER_PASS_SIMPLE_PASSES_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "render/frame.h"
#include "render/graph/pass.h"

// Small self-contained passes: IBL generation, environment maps,
// mip generation, skybox and post processing.

namespace rnd {

/* ==================================================
   ENVIRONMENT MAP
   ================================================== */

struct HdrToEnvPassData {
	const FrameParams *frame_params = nullptr;
	gfx::ResourceKey hdr_view;
};

void HdrToEnvPassFn(const PassContext &ctx);


/* ==================================================
   IBL
   ================================================== */

struct BRDFLutPassData {
	const FrameParams *frame_params = nullptr;
};

struct IBLPassIrradianceData {
	const FrameParams *frame_params = nullptr;
	gfx::ResourceKey env_view;
};

struct IBLPassPrefilterData {
	const FrameParams *frame_params = nullptr;
	gfx::ResourceKey env_view;
	float roughness = 0.f;
};

void BRDFLutPassFn(const PassContext &ctx);
void IBLPassIrradianceFn(const PassContext &ctx);
void IBLPassPrefilterFn(const PassContext &ctx);


/* ==================================================
   GENERATE MIPS
   ================================================== */

struct GenerateMipsPassData {
	gfx::ResourceKey texture;
};

void GenerateMipsPassFn(const PassContext &ctx);


/* ==================================================
   SKYBOX
   ================================================== */

struct SkyboxPassData {
	const FrameParams *frame_params = nullptr;
	gfx::ResourceKey cubemap;
};

void SkyboxPassFn(const PassContext &ctx);


/* ==================================================
   POST PROCESSING
   ================================================== */

struct PostProcessingPassData {
	const FrameParams *frame_params = nullptr;
	float exposure = 0.f;
	GraphTexHandle input;
	GraphTexHandle output;
	bool encode_srgb = false;
};

void PostProcessingPassFn(const PassContext &ctx);

}

#endif // RENDER_PASS_SIMPLE_PASSES_H

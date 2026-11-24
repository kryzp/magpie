#ifndef RENDER_PASS_FORWARD_H
#define RENDER_PASS_FORWARD_H

#include "core/types.h"
#include "render/frame.h"
#include "render/blackboard.h"
#include "render/pass/culling_pass.h"

namespace rnd {

class Graph;

struct DepthPrepassData {
	const FrameParams *frame_params;
	DrawStream draw_stream;
};

enum ForwardMode {
	ForwardMode_Opaque,
	ForwardMode_Translucent,
	ForwardMode_MaxEnum
};

struct ForwardPassData {
	const FrameParams *frame_params = nullptr;

	ForwardMode mode = ForwardMode_Opaque;

	u64 shadow_caster_table = 0;

	u64 irradiance_sh_buffer_address = 0;
	u64 irradiance_grid_info_buffer_address = 0;

	GraphTexHandle irradiance_fb_handle;
	GraphTexHandle prefilter_handle;
	GraphTexHandle brdf_handle;

	DrawStream opaque_draw_stream;
};

ForwardPassData *ForwardPassDataInit(
	Graph *graph, Pass *pass,
	const FrameParams *frame_params,
	const Blackboard *bb,
	ForwardMode mode
);

void DepthPrepass(
	Graph *graph,
	const FrameParams *fp,
	Blackboard *bb,
	const DrawStream &stream
);

void ForwardRender(
	Graph *graph,
	const FrameParams *frame_params,
	Blackboard *bb,
	const DrawStream &opaque_draw_stream
);

void ForwardRenderTranslucent(
	Graph *graph,
	const FrameParams *frame_params,
	Blackboard *bb
);

}

#endif // RENDER_PASS_FORWARD_H

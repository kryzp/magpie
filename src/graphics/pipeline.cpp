#include "graphics/pipeline.h"

#include <cstring>

gfx::GraphicsPipelineDef::GraphicsPipelineDef(ResourceKey program_)
	: colour_attachment_formats{}
{
	// we gotta set to zero including tail padding so that identical defs are the same
	memset(this, 0, sizeof(*this));

	program = program_;

	topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	cull_mode = VK_CULL_MODE_BACK_BIT;
	front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	blend_state = BlendState();
	depth_stencil_state = DepthStencilState();

	colour_attachment_count = 0;
	has_depth_attachment = false;
	depth_attachment_format = VK_FORMAT_UNDEFINED;

	depth_bias_enabled = false;
	depth_bias_constant = 0.f;
	depth_bias_slope = 0.f;

	samples = VK_SAMPLE_COUNT_1_BIT;
	min_sample_shading_enabled = true;
	min_sample_shading = 0.2f;

	multi_view_mask = 0;
}

gfx::GraphicsPipelineDef::GraphicsPipelineDef(ResourceKey program_, const RenderInfo &info)
	: GraphicsPipelineDef(program_)
{
	colour_attachment_count = info.colour_attachment_count;

	for (unsigned i = 0; i < colour_attachment_count; i++)
		colour_attachment_formats[i] = info.colour_attachment_formats[i];

	has_depth_attachment = info.has_depth_attachment;
	depth_attachment_format = info.depth_attachment_format;

	samples = info.samples;

	multi_view_mask = info.view_mask;
}

gfx::ComputePipelineDef::ComputePipelineDef(ResourceKey program_)
{
	// we gotta set to zero including tail padding so that identical defs are the same
	memset(this, 0, sizeof(*this));

	program = program_;
}

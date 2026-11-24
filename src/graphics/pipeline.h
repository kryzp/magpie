#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "graphics/blend.h"

#include <volk/volk.h>

#define GRAPHICS_MAX_COLOUR_ATTACHMENTS 8

namespace gfx {

struct RenderInfo {
	unsigned width = 0;
	unsigned height = 0;

	unsigned colour_attachment_count = 0;
	VkFormat colour_attachment_formats[GRAPHICS_MAX_COLOUR_ATTACHMENTS] = {};
	VkRenderingAttachmentInfo colour_attachments[GRAPHICS_MAX_COLOUR_ATTACHMENTS] = {};

	bool has_depth_attachment = false;
	VkFormat depth_attachment_format = VK_FORMAT_UNDEFINED;
	VkRenderingAttachmentInfo depth_attachment = {};

	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

	unsigned view_mask = 0;
};

// hashed byte for byte
struct GraphicsPipelineDef {
	ResourceKey program;

	VkPrimitiveTopology topology;
	VkCullModeFlags cull_mode;
	VkFrontFace front_face;

	BlendState blend_state;
	DepthStencilState depth_stencil_state;

	unsigned colour_attachment_count;
	VkFormat colour_attachment_formats[GRAPHICS_MAX_COLOUR_ATTACHMENTS];

	unsigned has_depth_attachment;
	VkFormat depth_attachment_format; // VK_FORMAT_UNDEFINED = fallbakc to device main depth-stencil format

	unsigned depth_bias_enabled;
	float depth_bias_constant;
	float depth_bias_slope;

	VkSampleCountFlagBits samples;
	unsigned min_sample_shading_enabled;
	float min_sample_shading;

	unsigned multi_view_mask;

	explicit GraphicsPipelineDef(ResourceKey program);
	GraphicsPipelineDef(ResourceKey program, const RenderInfo &info);
};

struct ComputePipelineDef {
	ResourceKey program;

	explicit ComputePipelineDef(ResourceKey program_);
};

struct PipelineState {
	ResourceKey pipeline;
	ResourceKey layout;
	VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
};
}

#endif // GRAPHICS_PIPELINE_H

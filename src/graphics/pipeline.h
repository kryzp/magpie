#pragma once

#include <volk/volk.h>

#include "container/vector.h"
#include "core/types.h"

#include "shader.h"
#include "blend.h"

namespace gfx
{
	struct RenderInfo {
		u32 width;
		u32 height;
		VkSampleCountFlags samples;
		u32 view_mask;
		Vector<VkRenderingAttachmentInfo> colour_attachments;
		VkRenderingAttachmentInfo depth_attachment;
	};

	struct GraphicsPipelineDef {
		const ShaderProgram &program;
		VkCullModeFlags cull_mode;
		VkFrontFace front_face;
		BlendState blend_state;
		DepthStencilState depth_stencil_state;
	
		// TODO: These things shouldn't be part of this and instead should be
		//       determined when the pipeline is created from some other extra
		//       parameters to the device functionlike the render info or
		//       something along those lines.
		Vector<VkFormat> colour_attachment_formats;
		bool has_depth_attachment;
	
		VkSampleCountFlagBits samples;
		bool min_sample_shading_enabled;
		float min_sample_shading;
		u32 view_mask;

		GraphicsPipelineDef(const ShaderProgram &program)
			: program(program)
			, cull_mode(VK_CULL_MODE_BACK_BIT)
			, front_face(VK_FRONT_FACE_CLOCKWISE)
			, blend_state()
			, depth_stencil_state()
			, has_depth_attachment(false)
			, samples(VK_SAMPLE_COUNT_1_BIT)
			, min_sample_shading_enabled(true)
			, min_sample_shading(0.2f)
			, view_mask(0)
		{
		}
	};

	struct ComputePipelineDef {
		const ShaderProgram &program;

		ComputePipelineDef(const ShaderProgram &program)
			: program(program)
		{
		}
	};

	struct PipelineState {
		VkPipeline pipeline;
		VkPipelineLayout layout;
		VkPipelineBindPoint bind_point;
	};
}

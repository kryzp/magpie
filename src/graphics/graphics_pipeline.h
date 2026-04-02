#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#define GFX_MAX_COLOUR_ATTACHMENTS 8

typedef struct GFX_RenderInfo GFX_RenderInfo;
struct GFX_RenderInfo
{
	u32 width;
	u32 height;
	VkSampleCountFlagBits samples;
	u32 view_mask;
	u32 colour_attachment_count;
	VkRenderingAttachmentInfo colour_attachments[GFX_MAX_COLOUR_ATTACHMENTS];
	VkRenderingAttachmentInfo depth_attachment;
};

typedef struct GFX_GraphicsPipelineDef GFX_GraphicsPipelineDef;
struct GFX_GraphicsPipelineDef
{
	GFX_ShaderKey program;

	VkPrimitiveTopology topology;
	VkCullModeFlags cull_mode;
	VkFrontFace front_face;

	GFX_BlendSt blend_state;
	GFX_DepthStencilSt depth_stencil_state;

	u32 colour_attachment_count;
	VkFormat colour_attachment_formats[GFX_MAX_COLOUR_ATTACHMENTS];

	b32 has_depth_attachment;

	VkSampleCountFlagBits samples;
	b32 min_sample_shading_enabled;
	f32 min_sample_shading;
	
	u32 multi_view_mask;
};

typedef struct GFX_ComputePipelineDef GFX_ComputePipelineDef;
struct GFX_ComputePipelineDef
{
	GFX_ShaderKey program;
};

internal GFX_GraphicsPipelineDef GFX_GraphicsPipelineDefInit(GFX_ShaderKey program);
internal GFX_ComputePipelineDef GFX_ComputePipelineDefInit(GFX_ShaderKey program);

typedef struct GFX_PipelineSt GFX_PipelineSt;
struct GFX_PipelineSt
{
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkPipelineBindPoint bind_point;
};

#endif // GRAPHICS_PIPELINE_H

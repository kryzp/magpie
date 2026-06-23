#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#define G_MAX_COLOUR_ATTACHMENTS 8

typedef struct G_RenderInfo G_RenderInfo;
struct G_RenderInfo
{
	u32 width;
	u32 height;

	u32 colour_attachment_count;
	VkFormat colour_attachment_formats[G_MAX_COLOUR_ATTACHMENTS];
	VkRenderingAttachmentInfo colour_attachments[G_MAX_COLOUR_ATTACHMENTS];

	b32 has_depth_attachment;
	VkRenderingAttachmentInfo depth_attachment;

	VkSampleCountFlagBits samples;

	u32 view_mask;
};

typedef struct G_GraphicsPipelineDef G_GraphicsPipelineDef;
struct G_GraphicsPipelineDef
{
	G_ShaderKey program;

	VkPrimitiveTopology topology;
	VkCullModeFlags cull_mode;
	VkFrontFace front_face;

	G_BlendSt blend_state;
	G_DepthStencilSt depth_stencil_state;

	u32 colour_attachment_count;
	VkFormat colour_attachment_formats[G_MAX_COLOUR_ATTACHMENTS];

	b32 has_depth_attachment;

	VkSampleCountFlagBits samples;
	b32 min_sample_shading_enabled;
	f32 min_sample_shading;
	
	u32 multi_view_mask;
};

typedef struct G_ComputePipelineDef G_ComputePipelineDef;
struct G_ComputePipelineDef
{
	G_ShaderKey program;
};

static G_GraphicsPipelineDef G_GraphicsPipelineDefInit     (G_ShaderKey program);
static G_GraphicsPipelineDef G_GraphicsPipelineDefFromInfo (G_ShaderKey program, const G_RenderInfo *info);

static G_ComputePipelineDef  G_ComputePipelineDefInit      (G_ShaderKey program);

#endif // GRAPHICS_PIPELINE_H

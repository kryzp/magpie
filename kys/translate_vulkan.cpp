#include "translate.h"

#include "core/common.h"

#include "image.h"
#include "image_view.h"

using namespace mgp;

// wanna kms

#define BEGIN_TMAP(fromType, toType) toType vk_translate::get##toType(fromType t) { toType deftype=(toType)0; switch (t) {
#define END_TMAP() } MGP_ERROR("FUCK"); return deftype; }
#define TMAP(from, to) case from: { return to; }

#define BEGIN_TFLAG(fromType, toType) toType vk_translate::get##toType(fromType fin) { toType fout = (toType)0;
#define END_TFLAG() return fout; }
#define TFLAG(from, to) if (fin & from) { fout |= to; }

VkPipelineColorBlendAttachmentState vk_translate::getVkPipelineColorBlendAttachmentState(const BlendState &blend)
{
	VkPipelineColorBlendAttachmentState st = {};
	
	st.colorWriteMask = 0;

	if (blend.writeMask[0]) st.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
	if (blend.writeMask[1]) st.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
	if (blend.writeMask[2]) st.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
	if (blend.writeMask[3]) st.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

	st.colorBlendOp = getVkBlendOp(blend.colour.op);
	st.srcColorBlendFactor = getVkBlendFactor(blend.colour.src);
	st.dstColorBlendFactor = getVkBlendFactor(blend.colour.dst);

	st.alphaBlendOp = getVkBlendOp(blend.alpha.op);
	st.srcAlphaBlendFactor = getVkBlendFactor(blend.alpha.src);
	st.dstAlphaBlendFactor = getVkBlendFactor(blend.alpha.dst);

	st.blendEnable = blend.enabled ? VK_TRUE : VK_FALSE;

	return st;
}

VkPipelineDepthStencilStateCreateInfo vk_translate::getVkPipelineDepthStencilStateCreateInfo(const DepthStencilState &state)
{
	VkPipelineDepthStencilStateCreateInfo st = {};

	st.depthTestEnable			= state.depthTestEnable;
	st.depthWriteEnable			= state.depthWriteEnable;
	st.depthCompareOp			= getVkCompareOp(state.depthCompareOp);
	st.depthBoundsTestEnable	= state.depthBoundsTestEnable;
	st.minDepthBounds			= state.minDepthBounds;
	st.maxDepthBounds			= state.maxDepthBounds;
	st.stencilTestEnable		= state.stencilTestEnable;

	st.front.failOp			= getVkStencilOp(state.stencilFront.failOp);
	st.front.passOp			= getVkStencilOp(state.stencilFront.passOp);
	st.front.depthFailOp	= getVkStencilOp(state.stencilFront.depthFailOp);
	st.front.compareOp		= getVkCompareOp(state.stencilFront.compareOp);
	st.front.compareMask	= state.stencilFront.compareMask;
	st.front.writeMask		= state.stencilFront.writeMask;
	st.front.reference		= state.stencilFront.reference;

	st.back.failOp			= getVkStencilOp(state.stencilBack.failOp);
	st.back.passOp			= getVkStencilOp(state.stencilBack.passOp);
	st.back.depthFailOp		= getVkStencilOp(state.stencilBack.depthFailOp);
	st.back.compareOp		= getVkCompareOp(state.stencilBack.compareOp);
	st.back.compareMask		= state.stencilBack.compareMask;
	st.back.writeMask		= state.stencilBack.writeMask;
	st.back.reference		= state.stencilBack.reference;

	return st;
}

// coding for like 6 hours now
// terrible
// will probably remove at some point
// dont know
// dont care really

VkRenderingAttachmentInfo _getVkRenderingAttachmentInfo(const RenderInfo::Attachment &a, bool isDepth)
{
	VkRenderingAttachmentInfo attachment = {};

	if (a.view)
	{
		ImageView *vkView = (ImageView *)a.view;
		Image *vkImage = (Image *)vkView->getImage();

		ImageView *vkResolveView = (ImageView *)a.resolve;

		attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		attachment.imageView = vkView->getHandle();
		attachment.imageLayout = vkImage->getLayout();
		attachment.loadOp = vk_translate::getVkAttachmentLoadOp(a.loadOp);
		attachment.storeOp = vk_translate::getVkAttachmentStoreOp(a.storeOp);

		if (vkResolveView)
		{
			Image *vkResolveImage = (Image *)vkResolveView->getImage();
		
			attachment.resolveMode = vk_translate::getVkResolveModeFlagBits(a.resolveMode);

			attachment.resolveImageView = vkResolveView->getHandle();
			attachment.resolveImageLayout = vkResolveImage->getLayout();
		}

		if (isDepth)
		{
			attachment.clearValue.depthStencil.depth = a.depthClear;
			attachment.clearValue.depthStencil.stencil = a.stencilClear;
		}
		else
		{
			glm::vec4 colourClear = a.colourClear.getPremultiplied().getDisplayColour();

			attachment.clearValue.color.float32[0] = colourClear.x;
			attachment.clearValue.color.float32[1] = colourClear.y;
			attachment.clearValue.color.float32[2] = colourClear.z;
			attachment.clearValue.color.float32[3] = colourClear.w;
		}
	}

	return attachment;
}

VkRenderingInfo vk_translate::getVkRenderingInfo(const RenderInfo &info)
{
	cauto &infoAttachments = info.getColourAttachments();

	std::vector<VkRenderingAttachmentInfo> vkAttachments(infoAttachments.size());

	for (int i = 0; i < infoAttachments.size(); i++) {
		vkAttachments[i] = _getVkRenderingAttachmentInfo(infoAttachments[i], false);
	}

	VkRenderingAttachmentInfo vkDepthAttachment = _getVkRenderingAttachmentInfo(info.getDepthAttachment(), true);

	VkRenderingInfo vkInfo = {};

	vkInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	vkInfo.renderArea.offset = { 0, 0 };
	vkInfo.renderArea.extent = { info.getWidth(), info.getHeight() };
	vkInfo.layerCount = 1;
	vkInfo.colorAttachmentCount = vkAttachments.size();
	vkInfo.pColorAttachments = vkAttachments.data();
	vkInfo.pDepthAttachment = info.getDepthAttachment().view ? &vkDepthAttachment : nullptr;
	vkInfo.pStencilAttachment = nullptr;

	return vkInfo;
}

BEGIN_TFLAG(VkBufferUsageFlags, VkVkBufferUsageFlags)
	TFLAG(BUFFER_USAGE_TRANSFER_SRC_BIT,			VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
	TFLAG(BUFFER_USAGE_TRANSFER_DST_BIT,			VK_BUFFER_USAGE_TRANSFER_DST_BIT)
	TFLAG(BUFFER_USAGE_STORAGE_BUFFER_BIT,			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
	TFLAG(BUFFER_USAGE_UNIFORM_BUFFER_BIT,			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
	TFLAG(BUFFER_USAGE_INDEX_BUFFER_BIT,			VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
	TFLAG(BUFFER_USAGE_VERTEX_BUFFER_BIT,			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
	TFLAG(BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,	VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
END_TFLAG()

BEGIN_TFLAG(VkDescriptorSetLayoutCreateFlags, VkDescriptorSetLayoutCreateFlags)
	TFLAG(DESCRIPTOR_LAYOUT_UPDATE_AFTER_BIND_POOL_BIT, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT)
END_TFLAG()

BEGIN_TFLAG(VkDescriptorPoolCreateFlags, VkDescriptorPoolCreateFlags)
	TFLAG(DESCRIPTOR_POOL_UPDATE_AFTER_BIND_BIT,		VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
	TFLAG(DESCRIPTOR_POOL_FREE_DESCRIPTOR_SET_BIT,		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
END_TFLAG()

BEGIN_TFLAG(VkVkDescriptorBindingFlags, VkVkDescriptorBindingFlags)
	TFLAG(DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT)
	TFLAG(DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT)
END_TFLAG()

BEGIN_TMAP(VkShaderStageFlags, VkShaderStageFlags)
	TMAP(SHADER_STAGE_VERTEX,			VK_SHADER_STAGE_VERTEX_BIT)
	TMAP(SHADER_STAGE_FRAGMENT,			VK_SHADER_STAGE_FRAGMENT_BIT)
	TMAP(SHADER_STAGE_ALL_GRAPHICS,		VK_SHADER_STAGE_ALL_GRAPHICS)
	TMAP(SHADER_STAGE_COMPUTE,			VK_SHADER_STAGE_COMPUTE_BIT)
END_TMAP()

BEGIN_TMAP(VkShaderStageFlags, VkShaderStageFlagBits)
	TMAP(SHADER_STAGE_VERTEX,			VK_SHADER_STAGE_VERTEX_BIT)
	TMAP(SHADER_STAGE_FRAGMENT,			VK_SHADER_STAGE_FRAGMENT_BIT)
	TMAP(SHADER_STAGE_ALL_GRAPHICS,		VK_SHADER_STAGE_ALL_GRAPHICS)
	TMAP(SHADER_STAGE_COMPUTE,			VK_SHADER_STAGE_COMPUTE_BIT)
END_TMAP()

BEGIN_TMAP(VmaAllocationCreateFlagBits, VmaAllocationCreateFlagBits)
	TMAP(ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
END_TMAP()

BEGIN_TMAP(VkPipelineBindPoint, VkPipelineBindPoint)
	TMAP(PIPELINE_BIND_POINT_GRAPHICS,		VK_PIPELINE_BIND_POINT_GRAPHICS)
	TMAP(PIPELINE_BIND_POINT_COMPUTE,		VK_PIPELINE_BIND_POINT_COMPUTE)
END_TMAP()

BEGIN_TMAP(VkCullModeFlags, VkCullModeFlags)
	TMAP(CULL_MODE_BACK,	VK_CULL_MODE_BACK_BIT)
	TMAP(CULL_MODE_FRONT,	VK_CULL_MODE_FRONT_BIT)
END_TMAP()

BEGIN_TMAP(VkFrontFace, VkFrontFace)
	TMAP(FRONT_FACE_CLOCKWISE,			VK_FRONT_FACE_CLOCKWISE)
	TMAP(FRONT_FACE_ANTI_CLOCKWISE,		VK_FRONT_FACE_COUNTER_CLOCKWISE)
END_TMAP()

BEGIN_TMAP(VkImageTiling, VkVkImageTiling)
	TMAP(IMAGE_TILING_OPTIMAL, VK_IMAGE_TILING_OPTIMAL)
END_TMAP()

BEGIN_TMAP(VkImageViewType, VkImageViewType)
	TMAP(IMAGE_VIEW_TYPE_2D,			VK_IMAGE_VIEW_TYPE_2D)
	TMAP(IMAGE_VIEW_TYPE_CUBE,			VK_IMAGE_VIEW_TYPE_CUBE)
	TMAP(IMAGE_VIEW_TYPE_1D_ARRAY,		VK_IMAGE_VIEW_TYPE_1D_ARRAY)
	TMAP(IMAGE_VIEW_TYPE_2D_ARRAY,		VK_IMAGE_VIEW_TYPE_2D_ARRAY)
END_TMAP()

BEGIN_TMAP(VkImageAspectFlags, VkImageAspectFlags)
	TMAP(IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_COLOR_BIT)
	TMAP(IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_ASPECT_DEPTH_BIT)
END_TMAP()

BEGIN_TMAP(VkSampleCountFlagBits, VkSampleCountFlagBits)
	TMAP(SAMPLE_COUNT_1_BIT, VK_SAMPLE_COUNT_1_BIT)
END_TMAP()

BEGIN_TMAP(VkFilter, VkFilter)
	TMAP(SAMPLE_FILTER_LINEAR,		VK_FILTER_LINEAR)
	TMAP(SAMPLE_FILTER_NEAREST,		VK_FILTER_NEAREST)
END_TMAP()

BEGIN_TMAP(VkSamplerAddressMode, VkSamplerAddressMode)
	TMAP(SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
END_TMAP()

BEGIN_TMAP(VkBorderColor, VkBorderColor)
	TMAP(BORDER_COLOUR_INT_OPAQUE_BLACK, VK_BORDER_COLOR_INT_OPAQUE_BLACK)
END_TMAP()

BEGIN_TMAP(VkLogicOp, VkLogicOp)
	TMAP(LOGIC_OP_COPY, VK_LOGIC_OP_COPY)
END_TMAP()

BEGIN_TMAP(VkCompareOp, VkCompareOp)
	TMAP(COMPARE_OP_LESS,				VK_COMPARE_OP_LESS)
	TMAP(COMPARE_OP_LESS_OR_EQUAL,		VK_COMPARE_OP_LESS_OR_EQUAL)
END_TMAP()

BEGIN_TMAP(VkStencilOp, VkStencilOp)
	TMAP(STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP)
END_TMAP()

BEGIN_TMAP(VkBlendOp, VkBlendOp)
	TMAP(BLEND_OP_ADD, VK_BLEND_OP_ADD)
END_TMAP()

BEGIN_TMAP(VkAttachmentLoadOp, VkAttachmentLoadOp)
	TMAP(ATTACHMENT_LOAD_OP_CLEAR,		VK_ATTACHMENT_LOAD_OP_CLEAR)
	TMAP(ATTACHMENT_LOAD_OP_LOAD,		VK_ATTACHMENT_LOAD_OP_LOAD)
END_TMAP()

BEGIN_TMAP(VkAttachmentStoreOp, VkAttachmentStoreOp)
	TMAP(ATTACHMENT_STORE_OP_STORE,		VK_ATTACHMENT_STORE_OP_STORE)
END_TMAP()

BEGIN_TMAP(VkBlendFactor, VkBlendFactor)
	TMAP(BLEND_FACTOR_ZERO,						VK_BLEND_FACTOR_ZERO)
	TMAP(BLEND_FACTOR_ONE,						VK_BLEND_FACTOR_ONE)
	TMAP(BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
	TMAP(BLEND_FACTOR_SRC_ALPHA,				VK_BLEND_FACTOR_SRC_ALPHA)
END_TMAP()

BEGIN_TMAP(VkResolveModeFlagBits, VkResolveModeFlagBits)
	TMAP(RESOLVE_MODE_NONE,					VK_RESOLVE_MODE_NONE)
	TMAP(RESOLVE_MODE_AVERAGE_BIT,			VK_RESOLVE_MODE_AVERAGE_BIT)
	TMAP(RESOLVE_MODE_SAMPLE_ZERO_BIT,		VK_RESOLVE_MODE_SAMPLE_ZERO_BIT)
END_TMAP()

BEGIN_TMAP(VkFormat, VkFormat)
	TMAP(GFX_FORMAT_UNDEFINED,				VK_FORMAT_UNDEFINED)
	TMAP(GFX_FORMAT_R32G32_SFLOAT,			VK_FORMAT_R32G32_SFLOAT)
	TMAP(GFX_FORMAT_R32G32B32A32_SFLOAT,	VK_FORMAT_R32G32B32A32_SFLOAT)
	TMAP(GFX_FORMAT_R32G32B32_SFLOAT,		VK_FORMAT_R32G32B32_SFLOAT)
	TMAP(GFX_FORMAT_R8G8B8A8_UNORM,			VK_FORMAT_R8G8B8A8_UNORM)
	TMAP(GFX_FORMAT_B8G8R8A8_UNORM,			VK_FORMAT_B8G8R8A8_UNORM)
	TMAP(GFX_FORMAT_D32_SFLOAT_S8_UINT,		VK_FORMAT_D32_SFLOAT_S8_UINT)
	TMAP(GFX_FORMAT_D24_UNORM_S8_UINT,		VK_FORMAT_D24_UNORM_S8_UINT)
END_TMAP()

BEGIN_TMAP(VkFormat, VkFormat)
	TMAP(VK_FORMAT_UNDEFINED,				GFX_FORMAT_UNDEFINED)
	TMAP(VK_FORMAT_R32G32_SFLOAT,			GFX_FORMAT_R32G32_SFLOAT)
	TMAP(VK_FORMAT_R32G32B32A32_SFLOAT,		GFX_FORMAT_R32G32B32A32_SFLOAT)
	TMAP(VK_FORMAT_R32G32B32_SFLOAT,		GFX_FORMAT_R32G32B32_SFLOAT)
	TMAP(VK_FORMAT_R8G8B8A8_UNORM,			GFX_FORMAT_R8G8B8A8_UNORM)
	TMAP(VK_FORMAT_B8G8R8A8_UNORM,			GFX_FORMAT_B8G8R8A8_UNORM)
	TMAP(VK_FORMAT_D32_SFLOAT_S8_UINT,		GFX_FORMAT_D32_SFLOAT_S8_UINT)
	TMAP(VK_FORMAT_D24_UNORM_S8_UINT,		GFX_FORMAT_D24_UNORM_S8_UINT)
END_TMAP()

BEGIN_TMAP(VkVertexInputRate, VkVertexInputRate)
	TMAP(VERTEX_INPUT_RATE_VERTEX,		VK_VERTEX_INPUT_RATE_VERTEX)
	TMAP(VERTEX_INPUT_RATE_INSTANCE,	VK_VERTEX_INPUT_RATE_INSTANCE)
END_TMAP()

BEGIN_TMAP(VkDescriptorType, VkDescriptorType)
	TMAP(DESCRIPTOR_TYPE_SAMPLER,						VK_DESCRIPTOR_TYPE_SAMPLER)
	TMAP(DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
	TMAP(DESCRIPTOR_TYPE_SAMPLED_IMAGE,					VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
	TMAP(DESCRIPTOR_TYPE_STORAGE_IMAGE,					VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
	TMAP(DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,			VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
	TMAP(DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,			VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
	TMAP(DESCRIPTOR_TYPE_UNIFORM_BUFFER,				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	TMAP(DESCRIPTOR_TYPE_STORAGE_BUFFER,				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
	TMAP(DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
	TMAP(DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
	TMAP(DESCRIPTOR_TYPE_INPUT_ATTACHMENT,				VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
END_TMAP()

BEGIN_TMAP(VkIndexType, VkIndexType)
	TMAP(INDEX_TYPE_UINT16, VK_INDEX_TYPE_UINT16)
END_TMAP()

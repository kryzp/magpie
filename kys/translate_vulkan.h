#pragma once

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "graphics/image.h"
#include "graphics/sampler.h"
#include "graphics/blend.h"
#include "graphics/shader.h"
#include "graphics/format.h"
#include "graphics/blend.h"
#include "graphics/gpu_buffer.h"
#include "graphics/descriptor.h"
#include "graphics/render_info.h"
#include "graphics/vertex_format.h"
#include "graphics/pipeline.h"
#include "graphics/command_buffer.h"

namespace mgp
{
	/*
	TODO: the flags() functions can't just use a switch-case-return, they need to actually build up the flags using OR's
	*/

	namespace vk_translate
	{
		VkVkBufferUsageFlags getVkVkBufferUsageFlags(VkBufferUsageFlags usage);

		VmaAllocationCreateFlagBits getVmaAllocationCreateFlagBits(VmaAllocationCreateFlagBits alloc);
		
		VkPipelineBindPoint getVkPipelineBindPoint(VkPipelineBindPoint bindPoint);
		VkCullModeFlags getVkCullModeFlags(VkCullModeFlags cull);
		VkFrontFace getVkFrontFace(VkFrontFace front);

		VkSampleCountFlagBits getVkSampleCountFlagBits(VkSampleCountFlagBits samples);
		VkFilter getVkFilter(VkFilter filter);
		VkSamplerAddressMode getVkSamplerAddressMode(VkSamplerAddressMode addressMode);
		VkBorderColor getVkBorderColor(VkBorderColor colour);
		
		VkLogicOp getVkLogicOp(VkLogicOp op);
		VkCompareOp getVkCompareOp(VkCompareOp op);
		VkStencilOp getVkStencilOp(VkStencilOp op);
		VkBlendOp getVkBlendOp(VkBlendOp op);
		VkAttachmentLoadOp getVkAttachmentLoadOp(VkAttachmentLoadOp op);
		VkAttachmentStoreOp getVkAttachmentStoreOp(VkAttachmentStoreOp op);

		VkBlendFactor getVkBlendFactor(VkBlendFactor factor);

		VkResolveModeFlagBits getVkResolveModeFlagBits(VkResolveModeFlagBits mode);

		VkFormat getGfxFormat(VkFormat format);
		VkFormat getVkFormat(VkFormat format);

		VkPipelineColorBlendAttachmentState getVkPipelineColorBlendAttachmentState(const BlendState &blend);
		VkPipelineDepthStencilStateCreateInfo getVkPipelineDepthStencilStateCreateInfo(const DepthStencilState &state);

		VkRenderingInfo getVkRenderingInfo(const RenderInfo &info);

		VkShaderStageFlags getVkShaderStageFlags(VkShaderStageFlags type);
		VkShaderStageFlagBits getVkShaderStageFlagBits(VkShaderStageFlags type);

		VkVertexInputRate getVkVertexInputRate(VkVertexInputRate rate);

		VkDescriptorType getVkDescriptorType(VkDescriptorType type);
		VkDescriptorSetLayoutCreateFlags getVkDescriptorSetLayoutCreateFlags(VkDescriptorSetLayoutCreateFlags flags);
		VkDescriptorPoolCreateFlags getVkDescriptorPoolCreateFlags(VkDescriptorPoolCreateFlags flags);
		VkVkDescriptorBindingFlags getVkVkDescriptorBindingFlags(VkVkDescriptorBindingFlags flags);

		VkIndexType getVkIndexType(VkIndexType type);
	}
}

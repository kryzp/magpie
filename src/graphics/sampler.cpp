#include "sampler.h"

#include "core/common.h"

#include "graphics_core.h"

using namespace mgp;

Sampler::Sampler(GraphicsCore *gfx, const SamplerStyle &style)
	: m_gfx(gfx)
	, m_sampler(VK_NULL_HANDLE)
	, m_style(style)
{
	cauto &properties = m_gfx->getPhysicalDeviceProperties().properties;

	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.minFilter = style.filter;
	createInfo.magFilter = style.filter;
	createInfo.addressModeU = style.wrapX;
	createInfo.addressModeV = style.wrapY;
	createInfo.addressModeW = style.wrapZ;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	createInfo.borderColor = style.borderColour;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = VK_LOD_CLAMP_NONE;

	MGP_VK_CHECK(
		vkCreateSampler(m_gfx->getLogicalDevice(), &createInfo, nullptr, &m_sampler),
		"Failed to create texture sampler"
	);
}

Sampler::~Sampler()
{
	vkDestroySampler(m_gfx->getLogicalDevice(), m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;
}

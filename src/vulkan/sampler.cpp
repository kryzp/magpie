#include "sampler.h"

#include "core/common.h"

#include "core.h"

using namespace mgp;

Sampler::Sampler(VulkanCore *core, const Sampler::Style &style)
	: m_sampler(VK_NULL_HANDLE)
	, m_style(style)
	, m_bindlessHandle(bindless::INVALID_HANDLE)
	, m_core(core)
{
	cauto &properties = m_core->getPhysicalDeviceProperties().properties;

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
		vkCreateSampler(m_core->getLogicalDevice(), &createInfo, nullptr, &m_sampler),
		"Failed to create texture sampler"
	);

	m_bindlessHandle = m_core->getBindlessResources().registerSampler(*this);
}

Sampler::~Sampler()
{
	vkDestroySampler(m_core->getLogicalDevice(), m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;
}

VkSampler Sampler::getHandle() const
{
	return m_sampler;
}

const Sampler::Style &Sampler::getStyle() const
{
	return m_style;
}

bindless::Handle Sampler::getBindlessHandle() const
{
	return m_bindlessHandle;
}

#include "texture_sampler.h"
#include "util.h"
#include "backend.h"

using namespace llt;

TextureSampler::TextureSampler()
	: dirty(true)
	, style()
	, m_sampler(VK_NULL_HANDLE)
{
}

TextureSampler::TextureSampler(const TextureSampler::Style& style)
	: dirty(true)
	, style(style)
	, m_sampler(VK_NULL_HANDLE)
{
}

TextureSampler::~TextureSampler()
{
	cleanUp();
}

void TextureSampler::cleanUp()
{
	if (m_sampler == VK_NULL_HANDLE) {
		return;
	}

	vkDestroySampler(g_vulkanBackend->device, m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;
}

VkSampler TextureSampler::bind(int maxMipLevels)
{
	// check if we actually need to create a new sampler or if our current one suffices
	if (!dirty) {
		return m_sampler;
	} else {
		dirty = false;
	}

	VkPhysicalDeviceProperties properties = g_vulkanBackend->physicalData.properties;

	cleanUp();

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
	createInfo.maxLod = (float)maxMipLevels;

	LLT_VK_CHECK(
		vkCreateSampler(g_vulkanBackend->device, &createInfo, nullptr, &m_sampler),
		"Failed to create texture sampler"
	);

	return m_sampler;
}

VkSampler TextureSampler::sampler() const
{
	return m_sampler;
}

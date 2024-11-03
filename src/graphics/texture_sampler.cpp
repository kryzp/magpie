#include "texture_sampler.h"
#include "util.h"
#include "backend.h"

using namespace llt;

TextureSampler::TextureSampler()
	: dirty(true)
	, m_sampler(VK_NULL_HANDLE)
{
}

TextureSampler::TextureSampler(const TextureSampler::Style& style)
	: dirty(true)
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

VkSampler TextureSampler::bind(VkDevice device, VkPhysicalDeviceProperties properties, int getMipLevels)
{
	// check if we actually need to create a new sampler or if our current one suffices
	if (!dirty) {
		return m_sampler;
	} else {
		dirty = false;
	}

	cleanUp();

	VkSamplerCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.minFilter = style.filter;
	create_info.magFilter = style.filter;
	create_info.addressModeU = style.wrapX;
	create_info.addressModeV = style.wrapY;
	create_info.addressModeW = style.wrapZ;
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	create_info.borderColor = style.borderColour;
	create_info.unnormalizedCoordinates = VK_FALSE;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.mipLodBias = 0.0f;
	create_info.minLod = 0.0f;
	create_info.maxLod = (float)getMipLevels;

	if (VkResult result = vkCreateSampler(device, &create_info, nullptr, &m_sampler); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:SAMPLER|DEBUG] Failed to create texture sampler: %d", result);
	}

	return m_sampler;
}

VkSampler TextureSampler::sampler() const { return m_sampler; }

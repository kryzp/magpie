#include "bindless.h"

#include <vector>

#include "core/common.h"

#include "core.h"
#include "gpu_buffer.h"
#include "sampler.h"
#include "image_view.h"

using namespace mgp;

void BindlessResources::init(VulkanCore *core)
{
	m_core = core;

	std::vector<DescriptorPoolSize> resources = {
		// samplers
		{
			VK_DESCRIPTOR_TYPE_SAMPLER,
			getMaxDescriptorSize(VK_DESCRIPTOR_TYPE_SAMPLER)
		},
		// 2d textures
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			getMaxDescriptorSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		},
		// cubemaps
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			getMaxDescriptorSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		}
	};

	DescriptorLayoutBuilder layoutBuilder;

	std::vector<VkDescriptorBindingFlags> flags;

	for (int i = 0; i < resources.size(); i++)
	{
		flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
		layoutBuilder.bind(i, resources[i].type, (uint32_t)resources[i].max);
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlags.bindingCount = flags.size();
	bindingFlags.pBindingFlags = flags.data();

	m_bindlessLayout = layoutBuilder.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS, &bindingFlags, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);
	m_bindlessPool.init(m_core, 1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, resources);
	m_bindlessSet = m_bindlessPool.allocate(m_bindlessLayout);
}

void BindlessResources::destroy()
{
	m_bindlessPool.cleanUp();
}

uint32_t BindlessResources::getMaxDescriptorSize(VkDescriptorType type)
{
	cauto &limits = m_core->getPhysicalDeviceProperties().properties.limits;

	const uint32_t HARD_CAP_ON_SIZE = 65536;

	switch (type)
	{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
			return CalcU::min(HARD_CAP_ON_SIZE, limits.maxDescriptorSetSamplers);

		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			return CalcU::min(HARD_CAP_ON_SIZE, limits.maxDescriptorSetSampledImages);
	}

	return 0;
}

uint32_t BindlessResources::registerSampler(const Sampler &sampler)
{
	uint32_t handle = m_samplerHandle_UID++;

	DescriptorWriter(m_core)
		.writeSampler(0, sampler, handle)
		.writeTo(m_bindlessSet);

	return handle;
}

uint32_t BindlessResources::registerTexture2D(const ImageView &view)
{
	uint32_t handle = m_textureHandle_UID++;

	DescriptorWriter(m_core)
		.writeSampledImage(1, view, handle)
		.writeTo(m_bindlessSet);

	return handle;
}

uint32_t BindlessResources::registerCubemap(const ImageView &cubemap)
{
	uint32_t handle = m_cubeHandle_UID++;

	DescriptorWriter(m_core)
		.writeSampledImage(2, cubemap, handle)
		.writeTo(m_bindlessSet);

	return handle;
}

const VkDescriptorSet &BindlessResources::getSet() const
{
	return m_bindlessSet;
}

const VkDescriptorSetLayout &BindlessResources::getLayout() const
{
	return m_bindlessLayout;
}

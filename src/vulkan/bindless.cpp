#include "bindless.h"

#include <vector>

#include "core/common.h"

#include "core.h"
#include "gpu_buffer.h"
#include "sampler.h"
#include "image_view.h"

using namespace mgp;

BindlessResources::BindlessResources()
	: m_core(nullptr)
	, m_bindlessPool()
	, m_bindlessSet()
	, m_bindlessLayout()
	, m_textureHandle_UID(0)
	, m_cubeHandle_UID(0)
	, m_samplerHandle_UID(0)
	, m_bufferHandle_UID(0)
{
}

BindlessResources::~BindlessResources()
{
}

void BindlessResources::init(VulkanCore *core)
{
	m_core = core;

	cauto &limits = core->getPhysicalDeviceProperties().properties.limits;

	const uint32_t HARD_CAP_ON_SIZE = 65536;

	std::vector<DescriptorPoolSize> resources = {
		// buffers
		{
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			CalcU::min(HARD_CAP_ON_SIZE, limits.maxDescriptorSetStorageBuffers)
		},
		// samplers
		{
			VK_DESCRIPTOR_TYPE_SAMPLER,
			CalcU::min(HARD_CAP_ON_SIZE, limits.maxDescriptorSetSamplers)
		},
		// 2d textures
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			CalcU::min(HARD_CAP_ON_SIZE, limits.maxDescriptorSetSampledImages)
		},
		// cubemaps
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			CalcU::min(HARD_CAP_ON_SIZE, limits.maxDescriptorSetSampledImages)
		}
	};

	std::vector<VkDescriptorBindingFlags> flags;
	DescriptorLayoutBuilder layoutBuilder;

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

bindless::Handle BindlessResources::registerBuffer(const GPUBuffer &buffer)
{
	bindless::Handle handle = m_bufferHandle_UID++;

	DescriptorWriter()
		.writeBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer.getDescriptorInfo(), handle)
		.updateSet(m_core, m_bindlessSet);

	return handle;
}

bindless::Handle BindlessResources::registerSampler(const Sampler &sampler)
{
	bindless::Handle handle = m_samplerHandle_UID++;

	DescriptorWriter()
		.writeSampler(1, sampler, handle)
		.updateSet(m_core, m_bindlessSet);

	return handle;
}

bindless::Handle BindlessResources::registerTexture2D(const ImageView &view)
{
	bindless::Handle handle = m_textureHandle_UID++;

	DescriptorWriter()
		.writeSampledImage(2, view, handle)
		.updateSet(m_core, m_bindlessSet);

	return handle;
}

bindless::Handle BindlessResources::registerCubemap(const ImageView &cubemap)
{
	bindless::Handle handle = m_cubeHandle_UID++;

	DescriptorWriter()
		.writeSampledImage(3, cubemap, handle)
		.updateSet(m_core, m_bindlessSet);

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

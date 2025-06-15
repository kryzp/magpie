#include "descriptor.h"

#include "core/common.h"

#include "math/calc.h"

#include "graphics_core.h"
#include "sampler.h"
#include "image.h"
#include "image_view.h"
#include "gpu_buffer.h"

using namespace mgp;

DescriptorPoolStatic::DescriptorPoolStatic(GraphicsCore *gfx, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes)
	: m_gfx(gfx)
	, m_pool(VK_NULL_HANDLE)
	, m_allocated()
{
	std::vector<VkDescriptorPoolSize> poolSizes(sizes.size());

	for (int i = 0; i < sizes.size(); i++)
	{
		poolSizes[i].type = sizes[i].type;
		poolSizes[i].descriptorCount = (uint32_t)(sizes[i].max * ((float)maxSets));
	}

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.flags = flags;
	poolCreateInfo.maxSets = maxSets;
	poolCreateInfo.poolSizeCount = poolSizes.size();
	poolCreateInfo.pPoolSizes = poolSizes.data();

	MGP_VK_CHECK(
		vkCreateDescriptorPool(m_gfx->getLogicalDevice(), &poolCreateInfo, nullptr, &m_pool),
		"Failed to create static descriptor pool"
	);
}

DescriptorPoolStatic::~DescriptorPoolStatic()
{
	if (m_pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_gfx->getLogicalDevice(), m_pool, nullptr);
		m_pool = VK_NULL_HANDLE;
	}

	for (auto &d : m_allocated)
	{
		delete d;
	}

	m_allocated.clear();
}

void DescriptorPoolStatic::clear()
{
	vkResetDescriptorPool(m_gfx->getLogicalDevice(), m_pool, 0);
}

Descriptor *DescriptorPoolStatic::allocate(const std::vector<DescriptorLayout *> &layouts)
{
	std::vector<VkDescriptorSetLayout> vkLayouts(layouts.size());

	for (int i = 0; i < layouts.size(); i++) {
		vkLayouts[i] = ((DescriptorLayout *)layouts[i])->getLayout();
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_pool;
	allocInfo.descriptorSetCount = vkLayouts.size();
	allocInfo.pSetLayouts = vkLayouts.data();

	VkDescriptorSet set = VK_NULL_HANDLE;

	MGP_VK_CHECK(
		vkAllocateDescriptorSets(m_gfx->getLogicalDevice(), &allocInfo, &set),
		"Failed to allocate descriptor set"
	);

	Descriptor *desc = new Descriptor(m_gfx, set);
	m_allocated.push_back(desc);

	return desc;
}

VkDescriptorPool DescriptorPoolStatic::getPool() const
{
	return m_pool;
}

// ack: mostly just vkguide, thank you!
DescriptorPool::DescriptorPool(GraphicsCore *gfx, uint32_t initialSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes)
	: m_gfx(gfx)
	, m_sizes(sizes)
	, m_flags(flags)
{
	VkDescriptorPool newPool = createNewPool(initialSets);

	m_freePools.push_back(newPool);
	m_setsPerPool = initialSets * GROWTH_FACTOR;
}

DescriptorPool::~DescriptorPool()
{
	for (auto &p : m_freePools)
	{
		vkDestroyDescriptorPool(m_gfx->getLogicalDevice(), p, nullptr);
	}

	for (auto &p : m_usedPools)
	{
		vkDestroyDescriptorPool(m_gfx->getLogicalDevice(), p, nullptr);
	}

	for (auto &d : m_allocated)
	{
		delete d;
	}

	m_freePools.clear();
	m_usedPools.clear();

	m_allocated.clear();
}

void DescriptorPool::clear()
{
	for (auto p : m_freePools)
	{
		vkResetDescriptorPool(m_gfx->getLogicalDevice(), p, 0);
	}

	for (auto p : m_usedPools)
	{
		vkResetDescriptorPool(m_gfx->getLogicalDevice(), p, 0);
		m_freePools.push_back(p);
	}

	m_usedPools.clear();

	for (auto &d : m_allocated)
	{
		delete d;
	}

	m_allocated.clear();
}

Descriptor *DescriptorPool::allocate(const std::vector<DescriptorLayout *> &layouts)
{
	std::vector<VkDescriptorSetLayout> vkLayouts(layouts.size());

	for (int i = 0; i < layouts.size(); i++) {
		vkLayouts[i] = ((DescriptorLayout *)layouts[i])->getLayout();
	}

	VkDescriptorPool allocatedPool = fetchPool();

	VkDescriptorSet set = VK_NULL_HANDLE;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = nullptr;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = allocatedPool;
	allocInfo.descriptorSetCount = vkLayouts.size();
	allocInfo.pSetLayouts = vkLayouts.data();

	VkResult result = vkAllocateDescriptorSets(m_gfx->getLogicalDevice(), &allocInfo, &set);

	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
	{
		m_usedPools.push_back(allocatedPool);

		VkDescriptorPool allocatedPool = fetchPool();
		allocInfo.descriptorPool = allocatedPool;

		MGP_VK_CHECK(
			vkAllocateDescriptorSets(m_gfx->getLogicalDevice(), &allocInfo, &set),
			"Failed to allocate descriptor sets"
		);
	}

	m_freePools.push_back(allocatedPool);

	Descriptor *desc = new Descriptor(m_gfx, set);
	m_allocated.push_back(desc);

	return desc;
}

VkDescriptorPool DescriptorPool::fetchPool()
{
	VkDescriptorPool newPool = VK_NULL_HANDLE;

	if (m_freePools.size() != 0)
	{
		newPool = m_freePools.back();
		m_freePools.pop_back();
	}
	else
	{
		newPool = createNewPool(m_setsPerPool);
		m_setsPerPool = CalcU::min(m_setsPerPool * GROWTH_FACTOR, 4092);
	}   

	return newPool;
}

VkDescriptorPool DescriptorPool::createNewPool(uint32_t maxSets)
{
	std::vector<VkDescriptorPoolSize> poolSizes(m_sizes.size());

	for (int i = 0; i < m_sizes.size(); i++)
	{
		poolSizes[i].type = m_sizes[i].type;
		poolSizes[i].descriptorCount = (uint32_t)(m_sizes[i].max * ((float)maxSets));
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = m_flags;
	pool_info.maxSets = maxSets;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	VkDescriptorPool newPool = {};

	vkCreateDescriptorPool(m_gfx->getLogicalDevice(), &pool_info, nullptr, &newPool);

	return newPool;
}

DescriptorLayout::DescriptorLayout(GraphicsCore *gfx, VkDescriptorSetLayout layout)
	: m_gfx(gfx)
	, m_layout(layout)
{
}

DescriptorLayout::~DescriptorLayout()
{
	vkDestroyDescriptorSetLayout(m_gfx->getLogicalDevice(), m_layout, nullptr);
}

Descriptor::Descriptor(GraphicsCore *gfx, const VkDescriptorSet &set)
	: m_gfx(gfx)
	, m_set(set)
	, m_writes()
	, m_dirty(false)
	, m_bufferInfos{}
	, m_imageInfos{}
	, m_nBufferInfos(0)
	, m_nImageInfos(0)
{
}

void Descriptor::clear()
{
	m_writes.clear();

	m_nBufferInfos = 0;
	m_nImageInfos = 0;

	m_dirty = true;
}

void Descriptor::update()
{
	vkUpdateDescriptorSets(
		m_gfx->getLogicalDevice(),
		m_writes.size(), m_writes.data(),
		0, nullptr
	);

	clear();
	
	m_dirty = false;
}

const VkDescriptorSet &Descriptor::getHandle()
{
	if (m_dirty)
		update();

	return m_set;
}

Descriptor *Descriptor::writeBufferVulkan(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorBufferInfo &info, uint32_t arrayIndex)
{
	if (m_nBufferInfos == MAX_BUFFER_INFOS-1)
		update();

	m_bufferInfos[m_nBufferInfos] = info;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = bindingIndex;
	write.dstSet = m_set;
	write.dstArrayElement = arrayIndex;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &m_bufferInfos[m_nBufferInfos];

	m_writes.push_back(write);

	m_nBufferInfos++;
	
	m_dirty = true;

	return this;
}

Descriptor *Descriptor::writeImageVulkan(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorImageInfo &info, uint32_t arrayIndex)
{
	if (m_nImageInfos == MAX_IMAGE_INFOS-1)
		update();

	m_imageInfos[m_nImageInfos] = info;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = bindingIndex;
	write.dstSet = m_set;
	write.dstArrayElement = arrayIndex;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &m_imageInfos[m_nImageInfos];

	m_writes.push_back(write);

	m_nImageInfos++;
	
	m_dirty = true;

	return this;
}

Descriptor *Descriptor::writeBuffer(uint32_t bindingIndex, const GPUBuffer *buffer, bool dynamic, uint32_t arrayIndex)
{
	VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

	if (dynamic)
	{
		if (buffer->isStorageBuffer())
			type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		
		if (buffer->isUniformBuffer())
			type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	}
	else
	{
		if (buffer->isStorageBuffer())
			type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		
		if (buffer->isUniformBuffer())
			type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}

	return writeBufferVulkan(bindingIndex, type, ((const GPUBuffer *)buffer)->getDescriptorInfo(), arrayIndex);
}

Descriptor *Descriptor::writeBufferRange(uint32_t bindingIndex, const GPUBuffer *buffer, bool dynamic, uint32_t range, uint32_t arrayIndex)
{
	VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

	if (dynamic)
	{
		if (buffer->isStorageBuffer())
			type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		
		if (buffer->isUniformBuffer())
			type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	}
	else
	{
		if (buffer->isStorageBuffer())
			type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		
		if (buffer->isUniformBuffer())
			type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}

	return writeBufferVulkan(bindingIndex, type, ((const GPUBuffer *)buffer)->getDescriptorInfoRange(range), arrayIndex);
}

Descriptor *Descriptor::writeCombinedImage(uint32_t bindingIndex, const ImageView *view, const Sampler *sampler, uint32_t arrayIndex)
{
	VkDescriptorImageInfo info = {};
	info.imageView = ((ImageView *)view)->getHandle();
	info.imageLayout = view->getImage()->isDepth() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	info.sampler = ((Sampler *)sampler)->getHandle();

	return writeImageVulkan(bindingIndex, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info, arrayIndex);
}

Descriptor *Descriptor::writeSampledImage(uint32_t bindingIndex, const ImageView *view, uint32_t arrayIndex)
{
	VkDescriptorImageInfo info = {};
	info.imageView = ((ImageView *)view)->getHandle();
	info.imageLayout = view->getImage()->isDepth() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	info.sampler = VK_NULL_HANDLE;

	return writeImageVulkan(bindingIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, info, arrayIndex);
}

Descriptor *Descriptor::writeStorageImage(uint32_t bindingIndex, const ImageView *view, uint32_t arrayIndex)
{
	VkDescriptorImageInfo info = {};
	info.imageView = ((ImageView *)view)->getHandle();
	info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.sampler = VK_NULL_HANDLE;

	return writeImageVulkan(bindingIndex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, info, arrayIndex);
}

Descriptor *Descriptor::writeSampler(uint32_t bindingIndex, const Sampler *sampler, uint32_t arrayIndex)
{
	VkDescriptorImageInfo info = {};
	info.imageView = VK_NULL_HANDLE;
	info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.sampler = ((Sampler *)sampler)->getHandle();

	return writeImageVulkan(bindingIndex, VK_DESCRIPTOR_TYPE_SAMPLER, info, arrayIndex);
}

void DescriptorLayoutCache::init(GraphicsCore *gfx)
{
	m_gfx = gfx;
}

void DescriptorLayoutCache::destroy()
{
	for (auto &[id, layout] : m_layoutCache)
		delete layout;

	m_layoutCache.clear();
}

DescriptorLayout *DescriptorLayoutCache::fetchLayout(
	VkShaderStageFlags shaderStages,
	const std::vector<DescriptorLayoutBinding> &bindings,
	VkDescriptorSetLayoutCreateFlags flags
)
{
	uint64_t hash = 0;

	hash::combine(&hash, &shaderStages);
	hash::combine(&hash, &flags);

	for (int i = 0; i < bindings.size(); i++)
		hash::combine(&hash, &bindings[i]);

	if (m_layoutCache.contains(hash))
	{
		return m_layoutCache[hash];
	}

	DescriptorLayout *layout = m_gfx->createDescriptorLayout(shaderStages, bindings, flags);

	m_layoutCache.insert({ hash, layout });

	return layout;
}

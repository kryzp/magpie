#include "bindless_resource_mgr.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "vulkan/texture.h"
#include "vulkan/texture_view.h"
#include "vulkan/texture_sampler.h"
#include "vulkan/gpu_buffer.h"

#include "gpu_buffer_mgr.h"

llt::BindlessResourceManager *llt::g_bindlessResources = nullptr;

using namespace llt;

BindlessResourceManager::BindlessResourceManager()
	: m_writer()
	, m_bindlessPool()
	, m_bindlessSet()
	, m_bindlessLayout()
	, m_textureHandle_UID(0)
	, m_cubeHandle_UID(0)
	, m_samplerHandle_UID(0)
	, m_bufferHandle_UID(0)
{
}

BindlessResourceManager::~BindlessResourceManager()
{
	m_bindlessPool.cleanUp();
}

void BindlessResourceManager::init()
{
	const uint32_t BINDLESS_MAX_BUFFERS = 65536;
	const uint32_t BINDLESS_MAX_SAMPLERS = 65536;
	const uint32_t BINDLESS_MAX_IMAGES = 65536;

	Vector<VkDescriptorBindingFlags> flags = {
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlags.bindingCount = flags.size();
	bindingFlags.pBindingFlags = flags.data();

	m_bindlessLayout = DescriptorLayoutBuilder()
		.bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		BINDLESS_MAX_BUFFERS)
		.bind(1, VK_DESCRIPTOR_TYPE_SAMPLER,			BINDLESS_MAX_SAMPLERS)
		.bind(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		BINDLESS_MAX_IMAGES)
		.bind(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		BINDLESS_MAX_IMAGES)
		.build(
			VK_SHADER_STAGE_ALL_GRAPHICS,
			&bindingFlags,
			VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
		);

	m_bindlessPool.init(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, {
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	(float)BINDLESS_MAX_BUFFERS },
		{ VK_DESCRIPTOR_TYPE_SAMPLER,			(float)BINDLESS_MAX_SAMPLERS },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		(float)BINDLESS_MAX_IMAGES },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		(float)BINDLESS_MAX_IMAGES }
	});

	m_bindlessSet = m_bindlessPool.allocate(m_bindlessLayout);
}

void BindlessResourceManager::updateSet()
{
	m_writer.updateSet(m_bindlessSet);
	m_writer.clear();
}

BindlessResourceHandle BindlessResourceManager::registerBuffer(const GPUBuffer *buffer)
{
	BindlessResourceHandle handle = {};
	handle.id = m_bufferHandle_UID++;

	writeBuffers(handle.id, { buffer });
	updateSet();

	return handle;
}

BindlessResourceHandle BindlessResourceManager::registerSampler(const TextureSampler *sampler)
{
	BindlessResourceHandle handle = {};
	handle.id = m_samplerHandle_UID++;

	writeSamplers(handle.id, { sampler });
	updateSet();

	return handle;
}

BindlessResourceHandle BindlessResourceManager::registerTexture2D(const TextureView &view)
{
	BindlessResourceHandle handle = {};
	handle.id = m_textureHandle_UID++;

	writeTexture2Ds(handle.id, { view });
	updateSet();

	return handle;
}

BindlessResourceHandle BindlessResourceManager::registerCubemap(const TextureView &cubemap)
{
	BindlessResourceHandle handle = {};
	handle.id = m_cubeHandle_UID++;

	writeCubemaps(handle.id, { cubemap });
	updateSet();

	return handle;
}

void BindlessResourceManager::writeBuffers(uint32_t firstIndex, const Vector<const GPUBuffer *> &buffers)
{
	for (int i = 0; i < buffers.size(); i++)
	{
		m_writer.writeBuffer(
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			buffers[i]->getDescriptorInfo(),
			firstIndex + i
		);
	}
}

void BindlessResourceManager::writeSamplers(uint32_t firstIndex, const Vector<const TextureSampler *> &samplers)
{
	for (int i = 0; i < samplers.size(); i++)
	{
		m_writer.writeSampler(
			1,
			samplers[i]->getHandle(),
			firstIndex + i
		);
	}
}

void BindlessResourceManager::writeTexture2Ds(uint32_t firstIndex, const Vector<TextureView> &views)
{
	for (int i = 0; i < views.size(); i++)
	{
		m_writer.writeSampledImage(
			2,
			views[i].getHandle(),
			VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			firstIndex + i
		);
	}
}

void BindlessResourceManager::writeCubemaps(uint32_t firstIndex, const Vector<TextureView> &cubemaps)
{
	for (int i = 0; i < cubemaps.size(); i++)
	{
		m_writer.writeSampledImage(
			3,
			cubemaps[i].getHandle(),
			VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			firstIndex + i
		);
	}
}

const VkDescriptorSet &BindlessResourceManager::getSet() const
{
	return m_bindlessSet;
}

const VkDescriptorSetLayout &BindlessResourceManager::getLayout() const
{
	return m_bindlessLayout;
}

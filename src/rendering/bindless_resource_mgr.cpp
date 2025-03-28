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
	, m_frameConstantsBuffer()
	, m_transformationBuffer()
	, m_bindlessPool()
	, m_bindlessSet()
	, m_bindlessLayout()
	, m_textureHandle_UID(0)
	, m_cubeHandle_UID(0)
	, m_samplerHandle_UID(0)
{
}

BindlessResourceManager::~BindlessResourceManager()
{
	delete m_frameConstantsBuffer;
	delete m_transformationBuffer;

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
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
	};

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlags.bindingCount = flags.size();
	bindingFlags.pBindingFlags = flags.data();

	m_bindlessLayout = DescriptorLayoutBuilder()
		.bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		1)
		.bind(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		1)
		.bind(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		BINDLESS_MAX_IMAGES)
		.bind(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		BINDLESS_MAX_IMAGES)
		.bind(4, VK_DESCRIPTOR_TYPE_SAMPLER,			BINDLESS_MAX_SAMPLERS)
		.build(
			VK_SHADER_STAGE_ALL_GRAPHICS,
			&bindingFlags,
			VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
		);

	m_bindlessPool.init(1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	(float)1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	(float)1 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		(float)BINDLESS_MAX_IMAGES },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		(float)BINDLESS_MAX_IMAGES },
		{ VK_DESCRIPTOR_TYPE_SAMPLER,			(float)BINDLESS_MAX_SAMPLERS }
	});

	m_bindlessSet = m_bindlessPool.allocate(m_bindlessLayout);

	m_frameConstantsBuffer = g_gpuBufferManager->createUniformBuffer(sizeof(FrameConstants));
	m_transformationBuffer = g_gpuBufferManager->createStorageBuffer(sizeof(TransformData) * 16);

	writeFrameConstants({
		.proj = glm::identity<glm::mat4>(),
		.view = glm::identity<glm::mat4>(),
		.cameraPosition = glm::zero<glm::vec4>()
		});

	for (int i = 0; i < 16; i++)
	{
		writeTransformData(i, {
			.model = glm::identity<glm::mat4>(),
			.normalMatrix = glm::identity<glm::mat4>()
			});
	}

	DescriptorWriter()
		.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_frameConstantsBuffer->getDescriptorInfo())
		.writeBuffer(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_transformationBuffer->getDescriptorInfo())
		.updateSet(m_bindlessSet);
}

void BindlessResourceManager::updateSet()
{
	m_writer.updateSet(m_bindlessSet);
	m_writer.clear();
}

void BindlessResourceManager::writeFrameConstants(const FrameConstants &frameConstants)
{
	m_frameConstantsBuffer->writeDataToMe(&frameConstants, sizeof(FrameConstants), 0);
}

void BindlessResourceManager::writeTransformData(int index, const TransformData &transformData)
{
	m_transformationBuffer->writeDataToMe(&transformData, sizeof(TransformData), sizeof(TransformData) * index);
}

BindlessResourceHandle BindlessResourceManager::registerTexture(TextureView &view)
{
	view.m_bindlessHandle = m_textureHandle_UID++;
	
	writeTexture2Ds(view.m_bindlessHandle, { view });
	updateSet();

	return view.m_bindlessHandle;
}

BindlessResourceHandle BindlessResourceManager::registerCubemap(TextureView &cubemap)
{
	cubemap.m_bindlessHandle = m_cubeHandle_UID++;

	writeCubemaps(cubemap.m_bindlessHandle, { cubemap });
	updateSet();

	return cubemap.m_bindlessHandle;
}

BindlessResourceHandle BindlessResourceManager::registerSampler(TextureSampler *sampler)
{
	sampler->m_bindlessHandle = m_samplerHandle_UID++;

	writeSamplers(sampler->m_bindlessHandle, { sampler });
	updateSet();

	return sampler->m_bindlessHandle;
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

void BindlessResourceManager::writeSamplers(uint32_t firstIndex, const Vector<TextureSampler *> &samplers)
{
	for (int i = 0; i < samplers.size(); i++)
	{
		m_writer.writeSampler(
			4,
			samplers[i]->getHandle(),
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

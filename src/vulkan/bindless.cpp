#include "bindless.h"

#include <vector>

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

	const uint32_t BINDLESS_MAX_BUFFERS = 65536;
	const uint32_t BINDLESS_MAX_SAMPLERS = 65536;
	const uint32_t BINDLESS_MAX_IMAGES = 65536;

	std::vector<VkDescriptorBindingFlags> flags = {
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
		.bind(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		BINDLESS_MAX_BUFFERS)	// buffers
		.bind(1, VK_DESCRIPTOR_TYPE_SAMPLER,			BINDLESS_MAX_SAMPLERS)	// samplers
		.bind(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		BINDLESS_MAX_IMAGES)	// 2d textures
		.bind(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		BINDLESS_MAX_IMAGES)	// cubemaps
		.build(
			m_core,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			&bindingFlags,
			VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
		);

	m_bindlessPool.init(m_core, 1, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, {
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	(float)BINDLESS_MAX_BUFFERS },	// buffers
		{ VK_DESCRIPTOR_TYPE_SAMPLER,			(float)BINDLESS_MAX_SAMPLERS },	// samplers
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		(float)BINDLESS_MAX_IMAGES },	// 2d textures
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		(float)BINDLESS_MAX_IMAGES }	// cubemaps
	});

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
		.writeBuffer(
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			buffer.getDescriptorInfo(),
			handle
		)
		.updateSet(m_core, m_bindlessSet);

	return handle;
}

bindless::Handle BindlessResources::registerSampler(const Sampler &sampler)
{
	bindless::Handle handle = m_samplerHandle_UID++;

	DescriptorWriter()
		.writeSampler(
			1,
			sampler.getHandle(),
			handle
		)
		.updateSet(m_core, m_bindlessSet);

	return handle;
}

bindless::Handle BindlessResources::registerTexture2D(const ImageView &view)
{
	bindless::Handle handle = m_textureHandle_UID++;

	DescriptorWriter()
		.writeSampledImage(
			2,
			view.getHandle(),
			VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			handle
		)
		.updateSet(m_core, m_bindlessSet);

	return handle;
}

bindless::Handle BindlessResources::registerCubemap(const ImageView &cubemap)
{
	bindless::Handle handle = m_cubeHandle_UID++;

	DescriptorWriter()
		.writeSampledImage(
			3,
			cubemap.getHandle(),
			VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			handle
		)
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

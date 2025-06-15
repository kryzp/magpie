#include "bindless.h"

#include "graphics/graphics_core.h"

using namespace mgp;

BindlessResources::BindlessResources(GraphicsCore *gfx)
{
	m_gfx = gfx;

	std::vector<DescriptorLayoutBinding> bindings =
	{
		// samplers
		{
			SAMPLER_BINDING,
			VK_DESCRIPTOR_TYPE_SAMPLER,
			getMaxDescriptorSize(VK_DESCRIPTOR_TYPE_SAMPLER),
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
		},

		// 2d textures
		{
			TEXTURE_2D_BINDING,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			getMaxDescriptorSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE),
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
		},
		
		// cubemaps
		{
			CUBEMAP_BINDING,
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			getMaxDescriptorSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE),
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
		}
	};

	std::vector<DescriptorPoolSize> sizes;

	for (auto &b : bindings)
	{
		sizes.push_back({ b.type, (float)b.count });
	}

	// yes this could (should) be done using the layout cache
	// but its minorly easier to just create it here and manage it manually
	// since this whole thing only needs a single layout anyway
	m_bindlessLayout = m_gfx->createDescriptorLayout(
		VK_SHADER_STAGE_ALL_GRAPHICS,
		bindings,
		VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
	);

	m_bindlessPool = m_gfx->createStaticDescriptorPool(
		1,
		VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		sizes
	);

	m_bindlessDesc = m_bindlessPool->allocate({ m_bindlessLayout });
}

BindlessResources::~BindlessResources()
{
	delete m_bindlessLayout;
	delete m_bindlessPool;
}

uint32_t BindlessResources::getMaxDescriptorSize(VkDescriptorType type)
{
	auto &limits = m_gfx->getPhysicalDeviceProperties().properties.limits;

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

BindlessHandle BindlessResources::fromSampler(const Sampler *sampler)
{
	uint32_t index = m_samplers.tryGetIndex(sampler);

	if (index != INVALID_HANDLE)
		return BindlessHandle(index);

	index = m_samplers.registerResource(sampler);
	m_bindlessDesc->writeSampler(SAMPLER_BINDING, sampler, index);

	return BindlessHandle(index);
}

BindlessHandle BindlessResources::fromTexture2D(const ImageView *view)
{
	uint32_t index = m_texture2Ds.tryGetIndex(view);

	if (index != INVALID_HANDLE)
		return BindlessHandle(index);

	index = m_texture2Ds.registerResource(view);
	m_bindlessDesc->writeSampledImage(TEXTURE_2D_BINDING, view, index);

	return BindlessHandle(index);
}

BindlessHandle BindlessResources::fromCubemap(const ImageView *cubemap)
{
	uint32_t index = m_cubemaps.tryGetIndex(cubemap);

	if (index != INVALID_HANDLE)
		return BindlessHandle(index);

	index = m_cubemaps.registerResource(cubemap);
	m_bindlessDesc->writeSampledImage(CUBEMAP_BINDING, cubemap, index);

	return BindlessHandle(index);
}

Descriptor *BindlessResources::getDescriptor()
{
	return m_bindlessDesc;
}

DescriptorLayout *BindlessResources::getLayout()
{
	return m_bindlessLayout;
}

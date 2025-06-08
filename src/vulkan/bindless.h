#pragma once

#include <Volk/volk.h>

#include "descriptor.h"

#include "math/calc.h"

namespace mgp
{
	class GPUBuffer;
	class Sampler;
	class ImageView;
	class VulkanCore;

	class BindlessResources
	{
		constexpr static uint32_t MAX_BUFFERS = 128;

	public:
		constexpr static uint32_t INVALID_HANDLE = Calc<uint32_t>::maxValue();

		BindlessResources() = default;
		~BindlessResources() = default;

		void init(VulkanCore *core);
		void destroy();

		uint32_t getMaxDescriptorSize(VkDescriptorType type);

		uint32_t registerSampler(const Sampler &sampler);
		uint32_t registerTexture2D(const ImageView &view);
		uint32_t registerCubemap(const ImageView &cubemap);

		const VkDescriptorSet &getSet() const;
		const VkDescriptorSetLayout &getLayout() const;

	private:
		VulkanCore *m_core;

		DescriptorPoolStatic m_bindlessPool;
		VkDescriptorSet m_bindlessSet;
		VkDescriptorSetLayout m_bindlessLayout;

		// todo: this should be more like a freelist
		uint32_t m_textureHandle_UID;
		uint32_t m_cubeHandle_UID;
		uint32_t m_samplerHandle_UID;
	};
}

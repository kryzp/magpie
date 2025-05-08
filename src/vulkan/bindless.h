#pragma once

#include <Volk/volk.h>

#include "descriptor.h"

namespace mgp
{
	namespace bindless
	{
		using Handle = int;

		constexpr static Handle INVALID_HANDLE = -1;
	}

	class GPUBuffer;
	class Sampler;
	class ImageView;
	class VulkanCore;

	class BindlessResources
	{
	public:
		BindlessResources();
		~BindlessResources();

		void init(VulkanCore *core);
		void destroy();

		bindless::Handle registerBuffer(const GPUBuffer &buffer);
		bindless::Handle registerSampler(const Sampler &sampler);
		bindless::Handle registerTexture2D(const ImageView &view);
		bindless::Handle registerCubemap(const ImageView &cubemap);

		const VkDescriptorSet &getSet() const;
		const VkDescriptorSetLayout &getLayout() const;

	private:
		VulkanCore *m_core;

		DescriptorPoolStatic m_bindlessPool;
		VkDescriptorSet m_bindlessSet;
		VkDescriptorSetLayout m_bindlessLayout;

		// todo: this should be more like a freelist
		bindless::Handle m_textureHandle_UID;
		bindless::Handle m_cubeHandle_UID;
		bindless::Handle m_samplerHandle_UID;
		bindless::Handle m_bufferHandle_UID;
	};
}

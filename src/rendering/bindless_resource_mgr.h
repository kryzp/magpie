#ifndef BINDLESS_RESOURCE_MGR_H_
#define BINDLESS_RESOURCE_MGR_H_

#include <glm/mat4x4.hpp>

#include "math/calc.h"

#include "vulkan/descriptor_allocator.h"
#include "vulkan/descriptor_builder.h"

#include "types.h"

namespace llt
{
	struct FrameConstants
	{
		glm::mat4 proj;
		glm::mat4 view;
		glm::vec4 cameraPosition;
//		Light lights[16];
	};

	struct TransformData
	{
		glm::mat4 model;
		glm::mat4 normalMatrix;
	};

	class Texture;
	class TextureView;
	class TextureSampler;
	class GPUBuffer;

	class BindlessResourceManager
	{
	public:
		BindlessResourceManager();
		~BindlessResourceManager();

		void init();

		void updateSet();

		void writeFrameConstants(const FrameConstants &frameConstants);
		void writeTransformData(int index, const TransformData &transformData);

		BindlessResourceHandle registerTexture(TextureView &view);
		BindlessResourceHandle registerCubemap(TextureView &cubemap);
		BindlessResourceHandle registerSampler(TextureSampler *sampler);

		void writeTexture2Ds(uint32_t firstIndex, const Vector<TextureView> &views);
		void writeCubemaps(uint32_t firstIndex, const Vector<TextureView> &cubemaps);
		void writeSamplers(uint32_t firstIndex, const Vector<TextureSampler *> &samplers);

		const VkDescriptorSet &getSet() const;
		const VkDescriptorSetLayout &getLayout() const;

	private:
		DescriptorWriter m_writer;

		GPUBuffer *m_frameConstantsBuffer;
		GPUBuffer *m_transformationBuffer;

		DescriptorPoolStatic m_bindlessPool;
		VkDescriptorSet m_bindlessSet;
		VkDescriptorSetLayout m_bindlessLayout;

		// todo: this should be more like a freelist
		BindlessResourceHandle m_textureHandle_UID;
		BindlessResourceHandle m_cubeHandle_UID;
		BindlessResourceHandle m_samplerHandle_UID;
	};

	extern BindlessResourceManager *g_bindlessResources;
}

#endif // BINDLESS_RESOURCE_MGR_H_

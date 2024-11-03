#ifndef UBO_MANAGER_H_
#define UBO_MANAGER_H_

#include "../common.h"
#include "../container/array.h"

#include "util.h"
#include "buffer.h"

namespace llt
{
	/**
	 * Abstracts away & manages the allocation
	 * and lifetime of a uniform buffer for use in shaders
	 */
	class UBOManager
	{
	public:
		UBOManager();

		/*
		 * Allocate a set amount of data for the shader parameters to use.
		 */
		void init(uint64_t initialSize);

		/*
		 * Cleans up the allocated memory.
		 */
		void cleanUp();

		/*
		 * Sends data to a specific shader.
		 */
		void push_data(VkShaderStageFlagBits shader, const void* data, uint64_t size, bool* modified);

		/*
		 * Re-Allocates more memory for the uniform buffer if we run out at any point.
		 */
		void reallocate_uniform_buffer(uint64_t size);

		/*
		 * Clears the ubo usage for this frame.
		 */
		void resetUboUsageInFrame();

		uint64_t getDescriptorCount() const;
		const VkDescriptorBufferInfo& getDescriptor(uint64_t idx) const;
		const Array<VkDescriptorBufferInfo, mgc::RASTER_SHADER_COUNT>& getDescriptors() const;

		const Array<uint32_t, mgc::RASTER_SHADER_COUNT>& getDynamicOffsets() const;
		uint64_t getDynamicOffsetCount() const;

	private:
		Buffer* m_ubo;
		Array<uint32_t, mgc::RASTER_SHADER_COUNT> m_uboDynamicOffsets;
		Array<VkDescriptorBufferInfo, mgc::RASTER_SHADER_COUNT> m_uboInfos;
		Array<uint64_t, mgc::FRAMES_IN_FLIGHT> m_uboUsageInFrame;
		uint64_t m_uboOffset;
		uint64_t m_uboMaxSize;
	};
}

#endif // UBO_MANAGER_H_

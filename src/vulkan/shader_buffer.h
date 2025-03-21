#ifndef DYNAMIC_SHADER_BUFFER_H_
#define DYNAMIC_SHADER_BUFFER_H_

#include "core/common.h"
#include "container/array.h"

#include "util.h"
#include "gpu_buffer.h"
#include "shader.h"

namespace llt
{
	class DynamicShaderBuffer
	{
	public:
		DynamicShaderBuffer();
		~DynamicShaderBuffer() = default;

		void init(uint64_t initialSize, VkDescriptorType type);
		void cleanUp();

		void pushData(const void *data, uint64_t size);

		void reallocateBuffer(uint64_t allocationSize);

		void resetBufferUsageInFrame();

		const VkDescriptorBufferInfo &getDescriptorInfo() const;
		VkDescriptorType getDescriptorType() const;

		uint32_t getDynamicOffset() const;

		GPUBuffer *getBuffer();
		const GPUBuffer *getBuffer() const;

	private:
		GPUBuffer *m_buffer;

		VkDescriptorBufferInfo m_info;
		uint32_t m_dynamicOffset;
		
		Array<uint64_t, mgc::FRAMES_IN_FLIGHT> m_usageInFrame;

		uint64_t m_offset;
		uint64_t m_maxSize;

		VkDescriptorType m_type;
	};
}

#endif // DYNAMIC_SHADER_BUFFER_H_

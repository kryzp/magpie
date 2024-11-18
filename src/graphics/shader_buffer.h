#ifndef UBO_MANAGER_H_
#define UBO_MANAGER_H_

#include "../common.h"
#include "../container/array.h"

#include "util.h"
#include "gpu_buffer.h"
#include "shader.h"

namespace llt
{
	enum ShaderBufferType
	{
		SHADER_BUFFER_NONE,
		SHADER_BUFFER_UBO,
		SHADER_BUFFER_SSBO,
	};

	/**
	 * Abstracts away & manages the allocation
	 * and lifetime of a uniform buffer for use in shaders
	 */
	class ShaderBuffer
	{
	public:
		ShaderBuffer();

		void init(uint64_t initialSize, ShaderBufferType type);
		void cleanUp();

		void pushData(ShaderParameters& params);
		void pushData(const void* data, uint64_t size);
		
		void reallocateBuffer(uint64_t size);

		void resetBufferUsageInFrame();

		const VkDescriptorBufferInfo& getDescriptor() const;
		VkDescriptorType getDescriptorType() const;

		void bind(uint32_t idx);
		void unbind();

		uint32_t getBoundIdx() const;
		bool isBound() const;

		uint32_t getDynamicOffset() const;

		GPUBuffer* getBuffer();
		const GPUBuffer* getBuffer() const;

	private:
		GPUBuffer* m_buffer;
		
		VkDescriptorBufferInfo m_info;
		uint32_t m_dynamicOffset;
		
		Array<uint64_t, mgc::FRAMES_IN_FLIGHT> m_usageInFrame;

		uint64_t m_offset;
		uint64_t m_maxSize;

		uint32_t m_boundIdx;
		bool m_isBound;
		
		ShaderBufferType m_type;
	};
}

#endif // UBO_MANAGER_H_

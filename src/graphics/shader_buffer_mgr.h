#ifndef UBO_MANAGER_H_
#define UBO_MANAGER_H_

#include "../common.h"
#include "../container/array.h"

#include "util.h"
#include "buffer.h"

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
	class ShaderBufferManager
	{
	public:
		ShaderBufferManager();

		void init(uint64_t initialSize, ShaderBufferType type);
		void cleanUp();

		void pushData(const void* data, uint64_t size, int currentFrame, bool* modified);
		
		void reallocateBuffer(uint64_t size);

		void resetBufferUsageInFrame(int currentFrame);

		const VkDescriptorBufferInfo& getDescriptor() const;

		uint32_t getDynamicOffset() const;

	private:
		Buffer* m_buffer;
		
		uint32_t m_dynamicOffset;
		VkDescriptorBufferInfo m_info;
		
		Array<uint64_t, mgc::FRAMES_IN_FLIGHT> m_usageInFrame;

		uint64_t m_offset;
		uint64_t m_maxSize;
		
		ShaderBufferType m_type;
	};
}

#endif // UBO_MANAGER_H_

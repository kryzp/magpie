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
		SHADER_BUFFER_MAX_ENUM
	};

	class DynamicShaderBuffer
	{
	public:
		DynamicShaderBuffer();

		void init(uint64_t initialSize, ShaderBufferType type);
		void cleanUp();

		ShaderParameters& getParameters();
		void setParameters(const ShaderParameters& params);

		void pushParameters();

		void reallocateBuffer(uint64_t size);

		void resetBufferUsageInFrame();

		const VkDescriptorBufferInfo& getDescriptorInfo() const;
		VkDescriptorType getDescriptorType() const;

		uint32_t getDynamicOffset() const;

		GPUBuffer* getBuffer();
		const GPUBuffer* getBuffer() const;

	private:
		void pushData(const void* data, uint64_t size);

		GPUBuffer* m_buffer;
		ShaderParameters m_parameters;

		VkDescriptorBufferInfo m_info;
		uint32_t m_dynamicOffset;
		
		Array<uint64_t, mgc::FRAMES_IN_FLIGHT> m_usageInFrame;

		uint64_t m_offset;
		uint64_t m_maxSize;

		ShaderBufferType m_type;
	};
}

#endif // UBO_MANAGER_H_

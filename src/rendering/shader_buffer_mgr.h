#ifndef SHADER_BUFFER_MGR_
#define SHADER_BUFFER_MGR_

#include "container/vector.h"

#include "vulkan/descriptor_builder.h"
#include "vulkan/shader_buffer.h"

namespace llt
{
	class ShaderBufferMgr
	{
	public:
		ShaderBufferMgr();
		~ShaderBufferMgr();

		void resetBufferUsageInFrame();

		DynamicShaderBuffer *createUBO(uint64_t perFrameSize);
		DynamicShaderBuffer *createSSBO(uint64_t perFrameSize);

	private:
		Vector<DynamicShaderBuffer*> m_buffers;
	};

	extern ShaderBufferMgr *g_shaderBufferManager;
}

#endif // SHADER_BUFFER_MGR_

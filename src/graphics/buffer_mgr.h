#ifndef VK_BUFFER_MGR_H_
#define VK_BUFFER_MGR_H_

#include "../container/vector.h"
#include "buffer.h"

namespace llt
{
	class BufferMgr
	{
	public:
		BufferMgr();
		~BufferMgr();

		Buffer* createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, uint64_t size);

		Buffer* createStagingBuffer(uint64_t size);
		Buffer* createVertexBuffer(uint64_t vertexCount);
		Buffer* createIndexBuffer(uint64_t indexCount);
		Buffer* createUBO(uint64_t size);
		Buffer* createSSBO(uint64_t size);

	private:
		Vector<Buffer*> m_vertexBuffers;
		Vector<Buffer*> m_indexBuffers;
	};

	extern BufferMgr* g_bufferManager;
}

#endif // VK_BUFFER_MGR_H_

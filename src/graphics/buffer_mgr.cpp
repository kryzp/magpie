#include "buffer_mgr.h"
#include "vertex.h"

llt::BufferMgr* llt::g_bufferManager = nullptr;

using namespace llt;

BufferMgr::BufferMgr()
	: m_vertexBuffers()
	, m_indexBuffers()
{
}

BufferMgr::~BufferMgr()
{
	for (auto& vertexBuffer : m_vertexBuffers) {
		delete vertexBuffer;
	}

	m_vertexBuffers.clear();

	for (auto& indexBuffer : m_indexBuffers) {
		delete indexBuffer;
	}

	m_indexBuffers.clear();
}

Buffer* BufferMgr::createStagingBuffer(uint64_t size)
{
	Buffer* staging_buffer = new Buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	staging_buffer->create(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size);
	return staging_buffer;
}

Buffer* BufferMgr::createVertexBuffer(uint64_t vertexCount)
{
	Buffer* getVertexBuffer = new Buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	getVertexBuffer->create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(Vertex) * vertexCount);
	m_vertexBuffers.pushBack(getVertexBuffer);
	return getVertexBuffer;
}

Buffer* BufferMgr::createIndexBuffer(uint64_t indexCount)
{
	Buffer* getIndexBuffer = new Buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	getIndexBuffer->create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uint16_t) * indexCount);
	m_indexBuffers.pushBack(getIndexBuffer);
	return getIndexBuffer;
}

Buffer* BufferMgr::createUniformBuffer(uint64_t size)
{
	Buffer* uniform_buffer = new Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	uniform_buffer->create(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size);
	return uniform_buffer;
}

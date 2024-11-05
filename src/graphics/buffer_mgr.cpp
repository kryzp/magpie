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

Buffer* BufferMgr::createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, uint64_t size)
{
	Buffer* buffer = new Buffer(usage);
	buffer->create(properties, size);
	return buffer;
}

Buffer* BufferMgr::createStagingBuffer(uint64_t size)
{
	Buffer* stagingBuffer = new Buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stagingBuffer->create(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size);
	return stagingBuffer;
}

Buffer* BufferMgr::createVertexBuffer(uint64_t vertexCount)
{
	Buffer* vertexBuffer = new Buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	vertexBuffer->create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(Vertex) * vertexCount);
	m_vertexBuffers.pushBack(vertexBuffer);
	return vertexBuffer;
}

Buffer* BufferMgr::createIndexBuffer(uint64_t indexCount)
{
	Buffer* indexBuffer = new Buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	indexBuffer->create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uint16_t) * indexCount);
	m_indexBuffers.pushBack(indexBuffer);
	return indexBuffer;
}

Buffer* BufferMgr::createUBO(uint64_t size)
{
	Buffer* uniformBuffer = new Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	uniformBuffer->create(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size);
	return uniformBuffer;
}

Buffer* BufferMgr::createSSBO(uint64_t size)
{
	Buffer* shaderStorageBuffer = new Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	shaderStorageBuffer->create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size);
	return shaderStorageBuffer;
}

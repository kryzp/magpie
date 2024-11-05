#include "gpu_buffer_mgr.h"

llt::GPUBufferMgr* llt::g_bufferManager = nullptr;

using namespace llt;

GPUBufferMgr::GPUBufferMgr()
	: m_vertexBuffers()
	, m_indexBuffers()
{
}

GPUBufferMgr::~GPUBufferMgr()
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

GPUBuffer* GPUBufferMgr::createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, uint64_t size)
{
	GPUBuffer* buffer = new GPUBuffer(usage);
	buffer->create(properties, size);
	return buffer;
}

GPUBuffer* GPUBufferMgr::createStagingBuffer(uint64_t size)
{
	GPUBuffer* stagingBuffer = new GPUBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stagingBuffer->create(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size);
	return stagingBuffer;
}

GPUBuffer* GPUBufferMgr::createVertexBuffer(uint64_t vertexCount, uint32_t vertexSize)
{
	GPUBuffer* vertexBuffer = new GPUBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	vertexBuffer->create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexSize * vertexCount);
	m_vertexBuffers.pushBack(vertexBuffer);
	return vertexBuffer;
}

GPUBuffer* GPUBufferMgr::createIndexBuffer(uint64_t indexCount)
{
	GPUBuffer* indexBuffer = new GPUBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	indexBuffer->create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uint16_t) * indexCount);
	m_indexBuffers.pushBack(indexBuffer);
	return indexBuffer;
}

GPUBuffer* GPUBufferMgr::createUBO(uint64_t size)
{
	GPUBuffer* uniformBuffer = new GPUBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	uniformBuffer->create(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size);
	return uniformBuffer;
}

GPUBuffer* GPUBufferMgr::createSSBO(uint64_t size)
{
	GPUBuffer* shaderStorageBuffer = new GPUBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	shaderStorageBuffer->create(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size);
	return shaderStorageBuffer;
}

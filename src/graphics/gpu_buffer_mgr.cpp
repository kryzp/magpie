#include "gpu_buffer_mgr.h"

llt::GPUBufferMgr* llt::g_gpuBufferManager = nullptr;

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

GPUBuffer* GPUBufferMgr::createBuffer(VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, uint64_t size)
{
	GPUBuffer* buffer = new GPUBuffer(usage, memoryUsage);
	buffer->create(size);
	return buffer;
}

GPUBuffer* GPUBufferMgr::createStagingBuffer(uint64_t size)
{
	GPUBuffer* stagingBuffer = new GPUBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	stagingBuffer->create(size);
	return stagingBuffer;
}

GPUBuffer* GPUBufferMgr::createVertexBuffer(uint64_t vertexCount, uint32_t vertexSize)
{
	GPUBuffer* vertexBuffer = new GPUBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	vertexBuffer->create(vertexSize * vertexCount);
	m_vertexBuffers.pushBack(vertexBuffer);
	return vertexBuffer;
}

GPUBuffer* GPUBufferMgr::createIndexBuffer(uint64_t indexCount)
{
	GPUBuffer* indexBuffer = new GPUBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	indexBuffer->create(sizeof(uint16_t) * indexCount);
	m_indexBuffers.pushBack(indexBuffer);
	return indexBuffer;
}

GPUBuffer* GPUBufferMgr::createUniformBuffer(uint64_t size)
{
	GPUBuffer* uniformBuffer = new GPUBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	uniformBuffer->create(size);
	return uniformBuffer;
}

GPUBuffer* GPUBufferMgr::createShaderStorageBuffer(uint64_t size)
{
	GPUBuffer* shaderStorageBuffer = new GPUBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT/* | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT*/, VMA_MEMORY_USAGE_CPU_TO_GPU);
	shaderStorageBuffer->create(size);
	return shaderStorageBuffer;
}

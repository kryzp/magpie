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
	return createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		size
	);
}

GPUBuffer* GPUBufferMgr::createVertexBuffer(uint64_t vertexCount, uint32_t vertexSize)
{
	GPUBuffer* vertexBuffer = createBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		vertexSize * vertexCount
	);

	m_vertexBuffers.pushBack(vertexBuffer);
	return vertexBuffer;
}

GPUBuffer* GPUBufferMgr::createIndexBuffer(uint64_t indexCount)
{
	GPUBuffer* indexBuffer = createBuffer(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		sizeof(uint16_t) * indexCount
	);

	m_indexBuffers.pushBack(indexBuffer);
	return indexBuffer;
}

GPUBuffer* GPUBufferMgr::createUniformBuffer(uint64_t size)
{
	return createBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		size
	);
}

GPUBuffer* GPUBufferMgr::createShaderStorageBuffer(uint64_t size)
{
	return createBuffer(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT/* | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT*/,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		size
	);
}

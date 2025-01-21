#include "sub_mesh.h"
#include "backend.h"

using namespace llt;

SubMesh::SubMesh()
	: m_parent(nullptr)
	, m_material(nullptr)
	, m_vertexBuffer(nullptr)
	, m_indexBuffer(nullptr)
	, m_nVertices(0)
	, m_nIndices(0)
{
}

SubMesh::~SubMesh()
{
}

void SubMesh::build(void* pVertices, uint32_t nVertices, uint64_t vertexSize, uint16_t* pIndices, uint32_t nIndices)
{
	m_nVertices = nVertices;
	m_nIndices = nIndices;

	uint64_t vertexBufferSize = nVertices * vertexSize;
	uint64_t indexBufferSize = nIndices * sizeof(uint16_t);

	// create the gpu buffers
	m_vertexBuffer = g_gpuBufferManager->createVertexBuffer(nVertices, vertexSize);
	m_indexBuffer = g_gpuBufferManager->createIndexBuffer(nIndices);

	// read data to the stage
	g_gpuBufferManager->meshStagingBuffer->writeDataToMe(pVertices, vertexBufferSize, 0);
	g_gpuBufferManager->meshStagingBuffer->writeDataToMe(pIndices, indexBufferSize, vertexBufferSize);

	// make the stage write that data to the gpu buffers
	g_gpuBufferManager->meshStagingBuffer->writeToBuffer(m_vertexBuffer, vertexBufferSize, 0, 0);
	g_gpuBufferManager->meshStagingBuffer->writeToBuffer(m_indexBuffer, indexBufferSize, vertexBufferSize, 0);
}

Mesh* SubMesh::getParent()
{
	return m_parent;
}

const Mesh* SubMesh::getParent() const
{
	return m_parent;
}

void SubMesh::setMaterial(Material* material)
{
	m_material = material;
}

Material* SubMesh::getMaterial()
{
	return m_material;
}

const Material* SubMesh::getMaterial() const
{
	return m_material;
}

GPUBuffer* SubMesh::getVertexBuffer() const
{
	return m_vertexBuffer;
}

GPUBuffer* SubMesh::getIndexBuffer() const
{
	return m_indexBuffer;
}

uint64_t SubMesh::getVertexCount() const
{
	return m_nVertices;
}

uint64_t SubMesh::getIndexCount() const
{
	return m_nIndices;
}

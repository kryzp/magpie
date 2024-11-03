#include "sub_mesh.h"
#include "backend.h"

using namespace llt;

SubMesh::SubMesh()
	: m_vertices()
	, m_indices()
	, m_vertexBuffer(nullptr)
	, m_indexBuffer(nullptr)
	, m_material(nullptr)
	, m_parent(nullptr)
{
}

SubMesh::~SubMesh()
{
}

void SubMesh::build(const Vector<Vertex>& vtx, const Vector<uint16_t>& idx)
{
	m_vertices = vtx;
	m_indices = idx;

	// create the gpu buffers
	m_vertexBuffer = g_bufferManager->createVertexBuffer(vtx.size());
	m_indexBuffer = g_bufferManager->createIndexBuffer(idx.size());

	// allocate a staging buffer for them
	Buffer* stage = g_bufferManager->createStagingBuffer(vtx.size() * sizeof(Vertex) + idx.size() * sizeof(uint16_t));

	// read data to the stage
	stage->readDataFromMemory(vtx.data(), vtx.size() * sizeof(Vertex), 0);
	stage->readDataFromMemory(idx.data(), idx.size() * sizeof(uint16_t), vtx.size() * sizeof(Vertex));

	// make the stage write that data to the gpu buffers
	stage->writeToBuffer(m_vertexBuffer, vtx.size() * sizeof(Vertex), 0, 0);
	stage->writeToBuffer(m_indexBuffer, idx.size() * sizeof(uint16_t), vtx.size() * sizeof(Vertex), 0);

	// finished, delete the stage
	delete stage;
}

const Mesh* SubMesh::getParent() const
{
	return m_parent;
}

void SubMesh::setMaterial(Material* getMaterial)
{
	m_material = getMaterial;
}

Material* SubMesh::getMaterial()
{
	return m_material;
}

const Material* SubMesh::getMaterial() const
{
	return m_material;
}

Buffer* SubMesh::getVertexBuffer() const
{
	return m_vertexBuffer;
}

Buffer* SubMesh::getIndexBuffer() const
{
	return m_indexBuffer;
}

Vector<Vertex>& SubMesh::getVertices()
{
	return m_vertices;
}

const Vector<Vertex>& SubMesh::getVertices() const
{
	return m_vertices;
}

uint64_t SubMesh::getVertexCount() const
{
	return m_vertices.size();
}

Vector<uint16_t>& SubMesh::getIndices()
{
	return m_indices;
}

const Vector<uint16_t>& SubMesh::getIndices() const
{
	return m_indices;
}

uint64_t SubMesh::getIndexCount() const
{
	return m_indices.size();
}

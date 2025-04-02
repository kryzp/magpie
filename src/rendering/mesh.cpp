#include "mesh.h"

#include "third_party/volk.h"

#include "vulkan/core.h"
#include "vulkan/command_buffer.h"
#include "vulkan/sync.h"
#include "vulkan/gpu_buffer.h"
#include "vulkan/vertex_format.h"

#include "material.h"

using namespace mgp;

Mesh::Mesh(VulkanCore *core)
	: m_owner(nullptr)
	, m_core(core)
	, m_subMeshes()
	, m_directory("")
{
}

Mesh::~Mesh()
{
	for (auto &sub : m_subMeshes)
		delete sub;
}

SubMesh *Mesh::createSubmesh()
{
	SubMesh *sub = new SubMesh(m_core);
	sub->m_parent = this;
	
	m_subMeshes.push_back(sub);

	return sub;
}

uint64_t Mesh::getSubmeshCount() const
{
	return m_subMeshes.size();
}

SubMesh *Mesh::getSubmesh(int idx) const
{
	return m_subMeshes[idx];
}

void Mesh::setOwner(RenderObject *owner)
{
	m_owner = owner;
}

RenderObject *Mesh::getOwner()
{
	return m_owner;
}

void Mesh::setDirectory(const std::string &directory)
{
	m_directory = directory;
}

const std::string &Mesh::getDirectory() const
{
	return m_directory;
}

SubMesh::SubMesh(VulkanCore *core)
	: m_parent(nullptr)
	, m_core(core)
	, m_vertexFormat(nullptr)
	, m_material(nullptr)
	, m_vertexBuffer(nullptr)
	, m_indexBuffer(nullptr)
	, m_nVertices(0)
	, m_nIndices(0)
{
}

SubMesh::~SubMesh()
{
	delete m_vertexBuffer;
	delete m_indexBuffer;
}

void SubMesh::build(
	const VertexFormat &format,
	void *pVertices, uint32_t nVertices,
	uint16_t *pIndices, uint32_t nIndices
)
{
	m_vertexFormat = &format;

	m_nVertices = nVertices;
	m_nIndices = nIndices;

	uint64_t vertexBufferSize = nVertices * m_vertexFormat->getVertexSize();
	uint64_t indexBufferSize = nIndices * sizeof(uint16_t);

	m_vertexBuffer = new GPUBuffer(
		m_core,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		vertexBufferSize
	);

	m_indexBuffer = new GPUBuffer(
		m_core,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		indexBufferSize
	);

	// read data to the stage (todo: yes, i know making a new staging buffer per submesh is idiotic, i just want to get this running quick and cba)
	GPUBuffer *stagingBuffer = new GPUBuffer(
		m_core,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		vertexBufferSize + indexBufferSize
	);

	stagingBuffer->writeDataToMe(pVertices, vertexBufferSize, 0);
	stagingBuffer->writeDataToMe(pIndices, indexBufferSize, vertexBufferSize);

	// make the stage write that data to the gpu buffers
	InstantSubmitSync instantSubmit(m_core);
	CommandBuffer &cmd = instantSubmit.begin();
	{
		stagingBuffer->writeToBuffer(cmd, m_vertexBuffer, vertexBufferSize, 0, 0);
		stagingBuffer->writeToBuffer(cmd, m_indexBuffer, indexBufferSize, vertexBufferSize, 0);
	}
	instantSubmit.submit();

	m_core->deviceWaitIdle();

	delete stagingBuffer;
}

void SubMesh::render(CommandBuffer &cmd) const
{
	cauto &bindings = m_vertexFormat->getBindingDescriptions();

	VkDeviceSize vertexBufferOffsets = 0;

	cmd.bindVertexBuffers(
		bindings[0].binding,
		1,
		&m_vertexBuffer->getHandle(),
		&vertexBufferOffsets
	);

	cmd.bindIndexBuffer(
		m_indexBuffer->getHandle(),
		0,
		VK_INDEX_TYPE_UINT16
	);

	cmd.drawIndexed(m_nIndices);
}

Mesh *SubMesh::getParent()
{
	return m_parent;
}

const Mesh *SubMesh::getParent() const
{
	return m_parent;
}

void SubMesh::setMaterial(Material *material)
{
	m_material = material;
}

Material *SubMesh::getMaterial()
{
	return m_material;
}

const Material *SubMesh::getMaterial() const
{
	return m_material;
}

GPUBuffer *SubMesh::getVertexBuffer() const
{
	return m_vertexBuffer;
}

GPUBuffer *SubMesh::getIndexBuffer() const
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

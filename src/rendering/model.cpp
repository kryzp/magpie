#include "model.h"

#include "third_party/volk.h"

#include "vulkan/core.h"
#include "vulkan/command_buffer.h"
#include "vulkan/sync.h"
#include "vulkan/gpu_buffer.h"
#include "vulkan/vertex_format.h"

#include "material.h"

using namespace mgp;

Model::Model(VulkanCore *core)
	: m_owner(nullptr)
	, m_core(core)
	, m_subMeshes()
	, m_directory("")
{
}

Model::~Model()
{
	for (auto &sub : m_subMeshes)
		delete sub;
}

Mesh *Model::createMesh()
{
	Mesh *sub = new Mesh(m_core);
	sub->m_parent = this;
	
	m_subMeshes.push_back(sub);

	return sub;
}

uint64_t Model::getSubmeshCount() const
{
	return m_subMeshes.size();
}

Mesh *Model::getSubmesh(int idx) const
{
	return m_subMeshes[idx];
}

void Model::setOwner(RenderObject *owner)
{
	m_owner = owner;
}

RenderObject *Model::getOwner()
{
	return m_owner;
}

void Model::setDirectory(const std::string &directory)
{
	m_directory = directory;
}

const std::string &Model::getDirectory() const
{
	return m_directory;
}

Mesh::Mesh(VulkanCore *core)
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

Mesh::~Mesh()
{
	delete m_vertexBuffer;
	delete m_indexBuffer;
}

void Mesh::build(
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
	GPUBuffer stagingBuffer(
		m_core,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		vertexBufferSize + indexBufferSize
	);

	stagingBuffer.write(pVertices, vertexBufferSize, 0);
	stagingBuffer.write(pIndices, indexBufferSize, vertexBufferSize);

	// make the stage write that data to the gpu buffers
	InstantSubmitSync instantSubmit(m_core);
	CommandBuffer &cmd = instantSubmit.begin();
	{
		cmd.copyBufferToBuffer(
			stagingBuffer,
			*m_vertexBuffer,
			{{
				.srcOffset = 0,
				.dstOffset = 0,
				.size = vertexBufferSize
			}}
		);

		cmd.copyBufferToBuffer(
			stagingBuffer,
			*m_indexBuffer,
			{{
				.srcOffset = vertexBufferSize,
				.dstOffset = 0,
				.size = indexBufferSize
			}}
		);
	}
	instantSubmit.submit();

	m_core->deviceWaitIdle();
}

void Mesh::bind(CommandBuffer &cmd) const
{
	cauto &bindings = m_vertexFormat->getBindingDescriptions();

	VkDeviceSize vertexBufferOffset = 0;

	cmd.bindVertexBuffers(
		bindings[0].binding,
		1,
		&m_vertexBuffer->getHandle(),
		&vertexBufferOffset
	);

	cmd.bindIndexBuffer(
		m_indexBuffer->getHandle(),
		0,
		VK_INDEX_TYPE_UINT16
	);
}

Model *Mesh::getParent()
{
	return m_parent;
}

const Model *Mesh::getParent() const
{
	return m_parent;
}

void Mesh::setMaterial(Material *material)
{
	m_material = material;
}

Material *Mesh::getMaterial()
{
	return m_material;
}

const Material *Mesh::getMaterial() const
{
	return m_material;
}

GPUBuffer *Mesh::getVertexBuffer() const
{
	return m_vertexBuffer;
}

GPUBuffer *Mesh::getIndexBuffer() const
{
	return m_indexBuffer;
}

uint64_t Mesh::getVertexCount() const
{
	return m_nVertices;
}

uint64_t Mesh::getIndexCount() const
{
	return m_nIndices;
}

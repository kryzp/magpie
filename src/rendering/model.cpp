#include "model.h"

#include "graphics/graphics_core.h"
#include "graphics/vertex_format.h"
#include "graphics/gpu_buffer.h"

using namespace mgp;

Model::Model(GraphicsCore *gfx)
	: m_gfx(gfx)
	, m_owner(nullptr)
	, m_meshes()
	, m_directory("NULLDIR")
{
}

Model::~Model()
{
	for (auto &m : m_meshes)
		delete m;

	m_meshes.clear();
}

Mesh *Model::createMesh()
{
	Mesh *sub = new Mesh(m_gfx);
	sub->m_parent = this;
	
	m_meshes.push_back(sub);

	return sub;
}

Mesh::Mesh(GraphicsCore *gfx)
	: m_gfx(gfx)
	, m_parent(nullptr)
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
	const VertexFormat *format,
	void *pVertices, uint32_t nVertices,
	uint16_t *pIndices, uint32_t nIndices
)
{
	m_vertexFormat = format;

	m_nVertices = nVertices;
	m_nIndices = nIndices;

	uint64_t vertexBufferSize = nVertices * m_vertexFormat->getVertexSize();
	uint64_t indexBufferSize = nIndices * sizeof(uint16_t);

	m_vertexBuffer = m_gfx->createGPUBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		vertexBufferSize
	);

	m_indexBuffer = m_gfx->createGPUBuffer(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		indexBufferSize
	);

	// read data to the stage (todo: yes, i know making a new staging buffer per submesh is idiotic, i just want to get this running quick and cba)
	GPUBuffer *stagingBuffer = m_gfx->createGPUBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		vertexBufferSize + indexBufferSize
	);

	stagingBuffer->write(pVertices, vertexBufferSize, 0);
	stagingBuffer->write(pIndices, indexBufferSize, vertexBufferSize);

	// make the stage write that data to the gpu buffers
	CommandBuffer *cmd = m_gfx->beginInstantSubmit();
	{
		cmd->copyBufferToBuffer(
			stagingBuffer,
			m_vertexBuffer,
			{{
				.srcOffset = 0,
				.dstOffset = 0,
				.size = vertexBufferSize
			}}
		);

		cmd->copyBufferToBuffer(
			stagingBuffer,
			m_indexBuffer,
			{{
				.srcOffset = vertexBufferSize,
				.dstOffset = 0,
				.size = indexBufferSize
			}}
		);
	}
	m_gfx->submit(cmd);

	// wait when staging buffer needs to delete
	// todo: yes, this is terrible and there really should just
	//       be a single staging buffer used for the whole program
	m_gfx->waitIdle();

	delete stagingBuffer;
}

void Mesh::bind(CommandBuffer *cmd) const
{
	VkDeviceSize vertexBufferOffset = 0;

	cmd->bindVertexBuffers(
		m_vertexFormat->getBindings()[0].binding,
		1,
		&m_vertexBuffer,
		&vertexBufferOffset
	);

	cmd->bindIndexBuffer(
		m_indexBuffer,
		0,
		VK_INDEX_TYPE_UINT16
	);
}

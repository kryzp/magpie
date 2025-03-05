#ifndef MESH_H_
#define MESH_H_

#include "core/common.h"

#include "container/vector.h"

#include "material.h"
#include "gpu_buffer_mgr.h"

namespace llt
{
	class Mesh;

	/**
	 * Generic mesh class for representing, storing and manipulating a mesh.
	 */
	class SubMesh
	{
		friend class Mesh;

	public:
		SubMesh();
		virtual ~SubMesh();

		void render(CommandBuffer &buffer) const;

		void build(const VertexFormat &format, void *pVertices, uint32_t nVertices, uint16_t *pIndices, uint32_t nIndices);

		Mesh *getParent();
		const Mesh *getParent() const;

		void setMaterial(Material *material);
		Material *getMaterial();
		const Material *getMaterial() const;

		GPUBuffer *getVertexBuffer() const;
		GPUBuffer *getIndexBuffer() const;

		uint64_t getVertexCount() const;
		uint64_t getIndexCount() const;

	private:
		Mesh *m_parent;
		const VertexFormat *m_vertexFormat;

		Material *m_material;

		GPUBuffer *m_vertexBuffer;
		GPUBuffer *m_indexBuffer;

		uint32_t m_nVertices;
		uint32_t m_nIndices;
	};
}

#endif // MESH_H_

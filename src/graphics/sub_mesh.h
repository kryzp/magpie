#ifndef MESH_H_
#define MESH_H_

#include "../common.h"

#include "../container/vector.h"

#include "material.h"
#include "vertex.h"
#include "buffer_mgr.h"
#include "buffer.h"

namespace llt
{
	class MeshMgr;
	class Mesh;

	/**
	 * Generic mesh class for representing, storing and manipulating a mesh.
	 */
	class SubMesh
	{
		friend class MeshMgr;
		friend class Mesh;

	public:
		SubMesh();
		virtual ~SubMesh();

		void build(const Vector<Vertex>& vtx, const Vector<uint16_t>& idx);

		const Mesh* getParent() const;

		void setMaterial(Material* getMaterial);
		Material* getMaterial();
		const Material* getMaterial() const;

		Buffer* getVertexBuffer() const;
		Buffer* getIndexBuffer() const;

		Vector<Vertex>& getVertices();
		const Vector<Vertex>& getVertices() const;

		uint64_t getVertexCount() const;

		Vector<uint16_t>& getIndices();
		const Vector<uint16_t>& getIndices() const;

		uint64_t getIndexCount() const;

	private:
		const Mesh* m_parent;

		Material* m_material;

		Buffer* m_vertexBuffer;
		Vector<Vertex> m_vertices;

		Buffer* m_indexBuffer;
		Vector<uint16_t> m_indices;
	};
}

#endif // MESH_H_

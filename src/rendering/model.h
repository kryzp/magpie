#pragma once

#include <string>
#include <vector>

namespace mgp
{
	class GPUBuffer;
	class CommandBuffer;
	class GraphicsCore;

	class Material;
	class Mesh;
	class RenderObject;
	class VertexFormat;

	class Model
	{
	public:
		Model(GraphicsCore *gfx);
		~Model();

		Mesh *createMesh();

		uint64_t getSubmeshCount() const { return m_meshes.size(); }
		Mesh *getSubmesh(int idx) const { return m_meshes[idx]; }

		void setOwner(RenderObject *owner) { m_owner = owner; }
		RenderObject *getOwner() { return m_owner; }

		void setDirectory(const std::string &directory) { m_directory = directory; }
		const std::string &getDirectory() const { return m_directory; }

	private:
		GraphicsCore *m_gfx;

		RenderObject *m_owner;
		std::vector<Mesh *> m_meshes;
		std::string m_directory;
	};

	class Mesh
	{
		friend class Model;

	public:
		Mesh(GraphicsCore *gfx);
		~Mesh();

		void build(
			const VertexFormat *format,
			void *pVertices, uint32_t nVertices,
			uint16_t *pIndices, uint32_t nIndices
		);

		void bind(CommandBuffer *cmd) const;

		Model *getParent() { return m_parent; }

		const VertexFormat *getVertexFormat() const { return m_vertexFormat; }

		void setMaterial(Material *material) { m_material = material; }
		Material *getMaterial() { return m_material; }

		GPUBuffer *getVertexBuffer() { return m_vertexBuffer; }
		GPUBuffer *getIndexBuffer() { return m_indexBuffer; }

		uint64_t getVertexCount() const { return m_nVertices; }
		uint64_t getIndexCount() const { return m_nIndices; }

	private:
		GraphicsCore *m_gfx;
		Model *m_parent;

		const VertexFormat *m_vertexFormat;

		Material *m_material;

		GPUBuffer *m_vertexBuffer;
		GPUBuffer *m_indexBuffer;

		uint32_t m_nVertices;
		uint32_t m_nIndices;
	};
}

#pragma once

#include <inttypes.h>
#include <string>
#include <vector>

namespace mgp
{
	class VulkanCore;
	class GPUBuffer;
	class CommandBuffer;
	class VertexFormat;
	class Material;
	class RenderObject;
	class Mesh;

	class Model
	{
	public:
		Model(VulkanCore *core);
		~Model();

		Mesh *createMesh();

		uint64_t getSubmeshCount() const;
		Mesh *getSubmesh(int idx) const;

		void setOwner(RenderObject *owner);
		RenderObject *getOwner();

		void setDirectory(const std::string &directory);
		const std::string &getDirectory() const;

	private:
		RenderObject *m_owner;

		VulkanCore *m_core;
		
		std::vector<Mesh *> m_subMeshes;

		std::string m_directory;
	};

	class Mesh
	{
		friend class Model;

	public:
		Mesh(VulkanCore *core);
		~Mesh();

		void build(const VertexFormat &format, void *pVertices, uint32_t nVertices, uint16_t *pIndices, uint32_t nIndices);

		void bind(CommandBuffer &cmd) const;

		Model *getParent();
		const Model *getParent() const;

		void setMaterial(Material *material);

		Material *getMaterial();
		const Material *getMaterial() const;

		GPUBuffer *getVertexBuffer() const;
		GPUBuffer *getIndexBuffer() const;

		uint64_t getVertexCount() const;
		uint64_t getIndexCount() const;

	private:
		Model *m_parent;

		VulkanCore *m_core;

		const VertexFormat *m_vertexFormat;

		Material *m_material;

		GPUBuffer *m_vertexBuffer;
		GPUBuffer *m_indexBuffer;

		uint32_t m_nVertices;
		uint32_t m_nIndices;
	};
}

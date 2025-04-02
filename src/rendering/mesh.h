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
	class SubMesh;

	class Mesh
	{
	public:
		Mesh(VulkanCore *core);
		~Mesh();

		SubMesh *createSubmesh();

		uint64_t getSubmeshCount() const;
		SubMesh *getSubmesh(int idx) const;

		void setOwner(RenderObject *owner);
		RenderObject *getOwner();

		void setDirectory(const std::string &directory);
		const std::string &getDirectory() const;

	private:
		RenderObject *m_owner;

		VulkanCore *m_core;
		
		std::vector<SubMesh *> m_subMeshes;

		std::string m_directory;
	};

	class SubMesh
	{
		friend class Mesh;

	public:
		SubMesh(VulkanCore *core);
		~SubMesh();

		void build(const VertexFormat &format, void *pVertices, uint32_t nVertices, uint16_t *pIndices, uint32_t nIndices);

		void render(CommandBuffer &cmd) const;

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

		VulkanCore *m_core;

		const VertexFormat *m_vertexFormat;

		Material *m_material;

		GPUBuffer *m_vertexBuffer;
		GPUBuffer *m_indexBuffer;

		uint32_t m_nVertices;
		uint32_t m_nIndices;
	};
}

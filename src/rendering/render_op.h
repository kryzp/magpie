#ifndef RENDER_OP_H_
#define RENDER_OP_H_

#include "sub_mesh.h"

namespace llt
{
	class GPUBuffer;

	struct RenderOp
	{
		uint32_t nVertices;
		const GPUBuffer *vertexBuffer;
		uint32_t nIndices;
		const GPUBuffer *indexBuffer;

		uint32_t instanceCount;
		uint32_t firstInstance;
		const GPUBuffer *instanceBuffer;

		uint32_t indirectDrawCount;
		uint32_t indirectOffset;
		const GPUBuffer *indirectBuffer;

		RenderOp()
			: nVertices(0)
			, vertexBuffer(nullptr)
			, nIndices(0)
			, indexBuffer(nullptr)
			, instanceCount(1)
			, firstInstance(0)
			, instanceBuffer(nullptr)
			, indirectDrawCount(0)
			, indirectOffset(0)
			, indirectBuffer(nullptr)
		{
		}

		RenderOp(const SubMesh &mesh)
			: RenderOp()
		{
			setMesh(mesh);
		}

		void setMesh(const SubMesh &mesh)
		{
			nVertices = mesh.getVertexCount();
			vertexBuffer = mesh.getVertexBuffer();

			nIndices = mesh.getIndexCount();
			indexBuffer = mesh.getIndexBuffer();
		}

		void setInstanceData(uint32_t count, uint32_t first, const GPUBuffer *buffer)
		{
			instanceCount = count;
			firstInstance = first;
			instanceBuffer = buffer;
		}

		void setIndirectData(uint32_t count, uint32_t offset, const GPUBuffer *buffer)
		{
			indirectDrawCount = count;
			indirectOffset = offset;
			indirectBuffer = buffer;
		}
	};
}

#endif // RENDER_OP_H_

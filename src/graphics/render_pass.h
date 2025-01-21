#ifndef RENDER_PASS_H_
#define RENDER_PASS_H_

#include "shader.h"
#include "gpu_buffer.h"
#include "sub_mesh.h"
#include "generic_render_target.h"

namespace llt
{
	struct RenderPass
	{
		uint32_t nVertices;
		const GPUBuffer* vertexBuffer;
		uint32_t nIndices;
		const GPUBuffer* indexBuffer;

		uint32_t instanceCount;
		uint32_t firstInstance;
		const GPUBuffer* instanceBuffer;

		uint32_t indirectDrawCount;
		uint32_t indirectOffset;
		const GPUBuffer* indirectBuffer;

		RenderPass()
			: nVertices()
			, vertexBuffer()
			, nIndices()
			, indexBuffer()
			, instanceCount()
			, firstInstance()
			, instanceBuffer()
			, indirectDrawCount()
			, indirectOffset()
			, indirectBuffer()
		{
			instanceCount = 1;
		}

		RenderPass(const SubMesh& mesh)
			: RenderPass()
		{
			setMesh(mesh);
		}

		void setMesh(const SubMesh& mesh)
		{
			nVertices = mesh.getVertexCount();
			vertexBuffer = mesh.getVertexBuffer();

			nIndices = mesh.getIndexCount();
			indexBuffer = mesh.getIndexBuffer();
		}

		void setInstanceData(uint32_t count, uint32_t first, const GPUBuffer* buffer)
		{
			instanceCount = count;
			firstInstance = first;
			instanceBuffer = buffer;
		}

		void setIndirectData(uint32_t count, uint32_t offset, const GPUBuffer* buffer)
		{
			indirectDrawCount = count;
			indirectOffset = offset;
			indirectBuffer = buffer;
		}
	};
}

#endif // RENDER_PASS_H_

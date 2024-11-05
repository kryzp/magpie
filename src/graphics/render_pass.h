#ifndef RENDER_PASS_H_
#define RENDER_PASS_H_

#include "shader.h"
#include "gpu_buffer.h"
#include "sub_mesh.h"
#include "generic_render_target.h"

namespace llt
{
	struct RenderOp
	{
		struct VertexData
		{
			uint32_t nVertices;
			const GPUBuffer* buffer;
		};

		struct IndexData
		{
			uint32_t nIndices;
			const GPUBuffer* buffer;
		};

		struct InstanceData
		{
			uint32_t instanceCount;
			uint32_t firstInstance;
			const GPUBuffer* buffer;
		};

		VertexData vertexData;
		IndexData indexData;
		InstanceData instanceData;

		RenderOp()
			: vertexData()
			, indexData()
			, instanceData()
		{
		}
	};

	class RenderPass
	{
	public:
		RenderPass()
			: instanceCount(1)
			, firstInstance(0)
			, instanceBuffer(nullptr)
			, mesh(nullptr)
		{
		}

		RenderOp build()
		{
			RenderOp operation;

			operation.vertexData.nVertices = mesh->getVertexCount();
			operation.vertexData.buffer = mesh->getVertexBuffer();

			operation.indexData.nIndices = mesh->getIndexCount();
			operation.indexData.buffer = mesh->getIndexBuffer();

			operation.instanceData.instanceCount = instanceCount;
			operation.instanceData.firstInstance = firstInstance;
			operation.instanceData.buffer = instanceBuffer;

			return operation;
		}

		uint32_t instanceCount;
		uint32_t firstInstance;
		const GPUBuffer* instanceBuffer;

		const SubMesh* mesh;
	};
}

#endif // RENDER_PASS_H_

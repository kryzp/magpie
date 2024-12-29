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
		struct MeshData
		{
			uint32_t nVertices;
			const GPUBuffer* vBuffer;
			uint32_t nIndices;
			const GPUBuffer* iBuffer;
		};

		struct InstanceData
		{
			uint32_t instanceCount;
			uint32_t firstInstance;
			const GPUBuffer* buffer;
		};

		struct IndirectData
		{
			uint32_t drawCount;
			uint32_t offset;
			const GPUBuffer* buffer;
		};

		MeshData meshData;
		InstanceData instanceData;
		IndirectData indirectData;

		RenderPass()
			: meshData()
			, instanceData()
			, indirectData()
		{
			instanceData.instanceCount = 1;
		}

		void setMesh(const SubMesh& mesh)
		{
			meshData.nVertices = mesh.getVertexCount();
			meshData.vBuffer = mesh.getVertexBuffer();

			meshData.nIndices = mesh.getIndexCount();
			meshData.iBuffer = mesh.getIndexBuffer();
		}

		void setInstanceData(uint32_t count, uint32_t first, const GPUBuffer* buffer)
		{
			instanceData.instanceCount = count;
			instanceData.firstInstance = first;
			instanceData.buffer = buffer;
		}

		void setIndirectData(uint32_t count, uint32_t offset, const GPUBuffer* buffer)
		{
			indirectData.drawCount = count;
			indirectData.offset = offset;
			indirectData.buffer = buffer;
		}
	};
}

#endif // RENDER_PASS_H_

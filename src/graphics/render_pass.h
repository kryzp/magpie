#ifndef RENDER_PASS_H_
#define RENDER_PASS_H_

#include "shader.h"
#include "buffer.h"
#include "vertex.h"
#include "sub_mesh.h"
#include "generic_render_target.h"

namespace llt
{
	/**
	 * Generic render operation that can be sent to the
	 * backend to be carried out and drawn to the screen.
	 */
	struct RenderOp
	{
		struct VertexData
		{
			const Vector<Vertex>* vertices;
			const Buffer* buffer;
		};

		struct IndexData
		{
			const Vector<uint16_t>* indices;
			const Buffer* buffer;
		};

		VertexData vertexData;
		IndexData indexData;

		RenderOp()
			: vertexData()
			, indexData()
		{
		}
	};

	/**
	 * Engine-Level Render operation basically
	 */
	class RenderPass
	{
	public:
		RenderPass()
		{
		}

		RenderOp build()
		{
			RenderOp operation;

			operation.vertexData.vertices = &mesh->getVertices();
			operation.vertexData.buffer = mesh->getVertexBuffer();

			operation.indexData.indices = &mesh->getIndices();
			operation.indexData.buffer = mesh->getIndexBuffer();

			return operation;
		}

		const SubMesh* mesh;
	};
}

#endif // RENDER_PASS_H_

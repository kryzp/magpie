#include "vertex_types.h"

mgp::VertexFormat mgp::vertex_types::PRIMITIVE_VERTEX_FORMAT;
mgp::VertexFormat mgp::vertex_types::PRIMITIVE_UV_VERTEX_FORMAT;
mgp::VertexFormat mgp::vertex_types::MODEL_VERTEX_FORMAT;

using namespace mgp;

void vertex_types::initVertexTypes()
{
	PRIMITIVE_VERTEX_FORMAT.setBindings(
		{
			VertexFormat::Binding(
				sizeof(PrimitiveVertex),
				VK_VERTEX_INPUT_RATE_VERTEX,
				{
					VertexFormat::Attribute(VK_FORMAT_R32G32B32_SFLOAT,		offsetof(PrimitiveVertex, position))
				}
			)
		}
	);

	PRIMITIVE_UV_VERTEX_FORMAT.setBindings(
		{
			VertexFormat::Binding(
				sizeof(PrimitiveUVVertex),
				VK_VERTEX_INPUT_RATE_VERTEX,
				{
					VertexFormat::Attribute(VK_FORMAT_R32G32B32_SFLOAT,		offsetof(PrimitiveUVVertex, position)),
					VertexFormat::Attribute(VK_FORMAT_R32G32_SFLOAT,		offsetof(PrimitiveUVVertex, uv))
				}
			)
		}
	);

	MODEL_VERTEX_FORMAT.setBindings(
		{
			VertexFormat::Binding(
				sizeof(ModelVertex),
				VK_VERTEX_INPUT_RATE_VERTEX,
				{
					VertexFormat::Attribute(VK_FORMAT_R32G32B32_SFLOAT,		offsetof(ModelVertex, position)),
					VertexFormat::Attribute(VK_FORMAT_R32G32_SFLOAT,		offsetof(ModelVertex, uv)),
					VertexFormat::Attribute(VK_FORMAT_R32G32B32_SFLOAT,		offsetof(ModelVertex, colour)),
					VertexFormat::Attribute(VK_FORMAT_R32G32B32_SFLOAT,		offsetof(ModelVertex, normal)),
					VertexFormat::Attribute(VK_FORMAT_R32G32B32_SFLOAT,		offsetof(ModelVertex, tangent)),
					VertexFormat::Attribute(VK_FORMAT_R32G32B32_SFLOAT,		offsetof(ModelVertex, bitangent))
				}
			)
		}
	);
}

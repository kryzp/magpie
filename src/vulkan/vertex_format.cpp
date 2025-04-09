#include "vertex_format.h"

mgp::VertexFormat mgp::vtx::PRIMITIVE_VERTEX_FORMAT;
mgp::VertexFormat mgp::vtx::PRIMITIVE_UV_VERTEX_FORMAT;
mgp::VertexFormat mgp::vtx::MODEL_VERTEX_FORMAT;

using namespace mgp;

void vtx::initVertexTypes()
{
	PRIMITIVE_VERTEX_FORMAT.setBindings(
		{
			{
				sizeof(PrimitiveVertex),
				VK_VERTEX_INPUT_RATE_VERTEX,
				{
					{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(PrimitiveVertex, position) }
				}
			}
		});

	PRIMITIVE_UV_VERTEX_FORMAT.setBindings(
		{
			{
				sizeof(PrimitiveUVVertex),
				VK_VERTEX_INPUT_RATE_VERTEX,
				{
					{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(PrimitiveUVVertex, position) },
					{ VK_FORMAT_R32G32_SFLOAT,		offsetof(PrimitiveUVVertex, uv) }
				}
			}
		});

	MODEL_VERTEX_FORMAT.setBindings(
		{
			{
				sizeof(ModelVertex),
				VK_VERTEX_INPUT_RATE_VERTEX,
				{
					{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, position) },
					{ VK_FORMAT_R32G32_SFLOAT,		offsetof(ModelVertex, uv) },
					{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, colour) },
					{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, normal) },
					{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, tangent) },
					{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, bitangent) }
				}
			}
		});
}

void VertexFormat::setBindings(const std::vector<Binding> &bindings)
{
	for (int i = 0; i < bindings.size(); i++)
	{
		cauto &binding = bindings[i];

		if (binding.rate == VK_VERTEX_INPUT_RATE_VERTEX)
			m_vertexSize = binding.stride;

		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = i;
		bindingDescription.stride = binding.stride;
		bindingDescription.inputRate = binding.rate;

		for (cauto &attribute : binding.attributes)
		{
			VkVertexInputAttributeDescription attributeDescription = {};
			attributeDescription.binding = i;
			attributeDescription.location = m_attributes.size();
			attributeDescription.format = attribute.format;
			attributeDescription.offset = attribute.offset;

			m_attributes.push_back(attributeDescription);
		}

		m_bindings.push_back(bindingDescription);
	}
}

const std::vector<VkVertexInputAttributeDescription> &VertexFormat::getAttributeDescriptions() const
{
	return m_attributes;
}

const std::vector<VkVertexInputBindingDescription> &VertexFormat::getBindingDescriptions() const
{
	return m_bindings;
}

uint64_t VertexFormat::getVertexSize() const
{
	return m_vertexSize;
}

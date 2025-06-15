#include "vertex_format.h"

using namespace mgp;

void VertexFormat::setBindings(const std::vector<Binding> &bindings)
{
	for (int i = 0; i < bindings.size(); i++)
	{
		Binding binding = bindings[i];

		if (binding.inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
			m_vertexSize = binding.stride;

		if (binding.inputRate == VK_VERTEX_INPUT_RATE_INSTANCE)
			m_instanceSize = binding.stride;

		binding.binding = i;

		for (Attribute attribute : binding.attributes)
		{
			attribute.binding = i;
			attribute.location = m_attributes.size();

			m_attributes.push_back(attribute);
		}

		m_bindings.push_back(binding);
	}
}

const std::vector<VertexFormat::Attribute> &VertexFormat::getAttributes() const
{
	return m_attributes;
}

const std::vector<VertexFormat::Binding> &VertexFormat::getBindings() const
{
	return m_bindings;
}

uint64_t VertexFormat::getVertexSize() const
{
	return m_vertexSize;
}

uint64_t VertexFormat::getInstanceSize() const
{
	return m_instanceSize;
}

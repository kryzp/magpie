#include "vertex_format.h"

mgp::VertexFormat mgp::vtx::PRIMITIVE_VERTEX_FORMAT;
mgp::VertexFormat mgp::vtx::PRIMTIIVE_UV_VERTEX_FORMAT;
mgp::VertexFormat mgp::vtx::MODEL_VERTEX_FORMAT;

using namespace mgp;

void vtx::initVertexTypes()
{
	PRIMITIVE_VERTEX_FORMAT.addBinding(sizeof(PrimitiveVertex), VK_VERTEX_INPUT_RATE_VERTEX, {
		{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(PrimitiveVertex, position) }
	});

	PRIMTIIVE_UV_VERTEX_FORMAT.addBinding(sizeof(PrimitiveUVVertex), VK_VERTEX_INPUT_RATE_VERTEX, {
		{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(PrimitiveUVVertex, position) },
		{ VK_FORMAT_R32G32_SFLOAT,		offsetof(PrimitiveUVVertex, uv) }
	});

	MODEL_VERTEX_FORMAT.addBinding(sizeof(ModelVertex), VK_VERTEX_INPUT_RATE_VERTEX, {
		{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, position) },
		{ VK_FORMAT_R32G32_SFLOAT,		offsetof(ModelVertex, uv) },
		{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, colour) },
		{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, normal) },
		{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, tangent) },
		{ VK_FORMAT_R32G32B32_SFLOAT,	offsetof(ModelVertex, bitangent) }
	});
}

VertexFormat::VertexFormat()
	: m_attributes()
	, m_bindings()
	, m_size(0)
{
}

VertexFormat::~VertexFormat()
{
}

void VertexFormat::addBinding(uint32_t stride, VkVertexInputRate inputRate, const std::vector<AttributeDescription> &attributes)
{
	if (inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
		m_size = stride;

	uint32_t binding = m_bindings.size();

	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = binding;
	bindingDescription.stride = stride;
	bindingDescription.inputRate = inputRate;

	for (auto &attrib : attributes)
	{
		VkVertexInputAttributeDescription attributeDescription = {};
		attributeDescription.binding = binding;
		attributeDescription.location = m_attributes.size();
		attributeDescription.format = attrib.format;
		attributeDescription.offset = attrib.offset;

		m_attributes.push_back(attributeDescription);
	}

	m_bindings.push_back(bindingDescription);
}

void VertexFormat::clearAttributes()
{
	m_attributes.clear();
}

void VertexFormat::clearBindings()
{
	m_bindings.clear();
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
	return m_size;
}

#include "vertex_descriptor.h"

using namespace llt;

VertexDescriptor::VertexDescriptor()
	: m_attributes()
	, m_bindings()
{
}

VertexDescriptor::~VertexDescriptor()
{
}

void VertexDescriptor::addAttribute(uint32_t binding, VkFormat format, uint32_t offset)
{
	VkVertexInputAttributeDescription attributeDescription;
	attributeDescription.binding = binding;
	attributeDescription.location = m_attributes.size();
	attributeDescription.format = format;
	attributeDescription.offset = offset;

	m_attributes.pushBack(attributeDescription);
}

void VertexDescriptor::addBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
{
	VkVertexInputBindingDescription bindingDescription;
	bindingDescription.binding = binding;
	bindingDescription.stride = stride;
	bindingDescription.inputRate = inputRate;

	m_bindings.pushBack(bindingDescription);
}

void VertexDescriptor::clearAttributes()
{
	m_attributes.clear();
}

void VertexDescriptor::clearBindings()
{
	m_bindings.clear();
}

const Vector<VkVertexInputAttributeDescription>& VertexDescriptor::getAttributeDescriptions() const
{
	return m_attributes;
}

const Vector<VkVertexInputBindingDescription>& VertexDescriptor::getBindingDescriptions() const
{
	return m_bindings;
}

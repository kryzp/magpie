#ifndef VERTEX_DESCRIPTOR_H_
#define VERTEX_DESCRIPTOR_H_

#include <vulkan/vulkan.h>

#include "../common.h"
#include "../container/vector.h"

namespace llt
{
	struct AttributeDescription
	{
		VkFormat format;
		uint32_t offset;
	};

	class VertexDescriptor
	{
	public:
		VertexDescriptor();
		~VertexDescriptor();

		void addBinding(uint32_t stride, VkVertexInputRate inputRate, const Vector<AttributeDescription>& attributes);

		void clearAttributes();
		void clearBindings();
		
		const Vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() const;
		const Vector<VkVertexInputBindingDescription>& getBindingDescriptions() const;

	private:
		Vector<VkVertexInputAttributeDescription> m_attributes;
		Vector<VkVertexInputBindingDescription> m_bindings;
	};
}

#endif // VERTEX_DESCRIPTOR_H_

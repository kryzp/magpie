#ifndef VERTEX_DESCRIPTOR_H_
#define VERTEX_DESCRIPTOR_H_

#include <vulkan/vulkan.h>

#include "../common.h"
#include "../container/vector.h"

namespace llt
{
	class VertexDescriptor
	{
	public:
		VertexDescriptor();
		~VertexDescriptor();

		void addAttribute(uint32_t binding, VkFormat format, uint32_t offset);
		void addBinding(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);

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

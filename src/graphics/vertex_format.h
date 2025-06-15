#pragma once

#include <vector>

#include <Volk/volk.h>

namespace mgp
{
	class VertexFormat
	{
	public:
		struct Attribute
		{
			uint32_t binding;
			uint32_t location;

			VkFormat format;
			uint32_t offset;

			Attribute(VkFormat f, uint32_t o)
				: binding(0)
				, location(0)
				, format(f)
				, offset(o)
			{
			}
		};

		struct Binding
		{
			uint32_t binding;

			uint32_t stride;
			VkVertexInputRate inputRate;

			std::vector<Attribute> attributes;

			Binding(uint32_t s, VkVertexInputRate i, const std::vector<Attribute> &a)
				: binding(0)
				, stride(s)
				, inputRate(i)
				, attributes(a)
			{
			}
		};

		void setBindings(const std::vector<Binding> &bindings);

		const std::vector<Attribute> &getAttributes() const;
		const std::vector<Binding> &getBindings() const;

		uint64_t getVertexSize() const;
		uint64_t getInstanceSize() const;

	private:
		std::vector<Attribute> m_attributes;
		std::vector<Binding> m_bindings;

		uint64_t m_vertexSize;
		uint64_t m_instanceSize;
	};
}

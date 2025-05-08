#pragma once

#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <Volk/volk.h>

#include "core/common.h"

namespace mgp
{
	class VertexFormat
	{
	public:
		struct Attribute
		{
			VkFormat format;
			uint32_t offset;
		};

		struct Binding
		{
			uint32_t stride;
			VkVertexInputRate rate;
			std::vector<Attribute> attributes;
		};

		VertexFormat() = default;
		~VertexFormat() = default;

		void setBindings(const std::vector<Binding> &bindings);

		const std::vector<VkVertexInputAttributeDescription> &getAttributeDescriptions() const;
		const std::vector<VkVertexInputBindingDescription> &getBindingDescriptions() const;

		uint64_t getVertexSize() const;

	private:
		std::vector<VkVertexInputAttributeDescription> m_attributes;
		std::vector<VkVertexInputBindingDescription> m_bindings;

		uint64_t m_vertexSize;
	};

	namespace vtx
	{
		struct PrimitiveVertex
		{
			glm::vec3 position;
		};

		struct PrimitiveUVVertex
		{
			glm::vec3 position;
			glm::vec2 uv;
		};

		struct ModelVertex
		{
			glm::vec3 position;
			glm::vec2 uv;
			glm::vec3 colour;
			glm::vec3 normal;
			glm::vec3 tangent;
			glm::vec3 bitangent;
		};

		extern VertexFormat PRIMITIVE_VERTEX_FORMAT;
		extern VertexFormat PRIMITIVE_UV_VERTEX_FORMAT;
		extern VertexFormat MODEL_VERTEX_FORMAT;

		void initVertexTypes();
	}
}

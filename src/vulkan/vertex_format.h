#ifndef VERTEX_DESCRIPTOR_H_
#define VERTEX_DESCRIPTOR_H_

#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "third_party/volk.h"

#include "core/common.h"

namespace mgp
{
	struct AttributeDescription
	{
		VkFormat format;
		uint32_t offset;
	};

	class VertexFormat
	{
	public:
		VertexFormat();
		~VertexFormat();

		void addBinding(uint32_t stride, VkVertexInputRate inputRate, const std::vector<AttributeDescription> &attributes);

		void clearAttributes();
		void clearBindings();
		
		const std::vector<VkVertexInputAttributeDescription> &getAttributeDescriptions() const;
		const std::vector<VkVertexInputBindingDescription> &getBindingDescriptions() const;

		uint64_t getVertexSize() const;

	private:
		std::vector<VkVertexInputAttributeDescription> m_attributes;
		std::vector<VkVertexInputBindingDescription> m_bindings;

		uint64_t m_size;
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
		extern VertexFormat PRIMTIIVE_UV_VERTEX_FORMAT;
		extern VertexFormat MODEL_VERTEX_FORMAT;

		void initVertexTypes();
	}
}

#endif // VERTEX_DESCRIPTOR_H_

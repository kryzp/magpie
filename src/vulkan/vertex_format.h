#ifndef VERTEX_DESCRIPTOR_H_
#define VERTEX_DESCRIPTOR_H_

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "third_party/volk.h"

#include "core/common.h"

#include "container/vector.h"

namespace llt
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

		void addBinding(uint32_t stride, VkVertexInputRate inputRate, const Vector<AttributeDescription>& attributes);

		void clearAttributes();
		void clearBindings();
		
		const Vector<VkVertexInputAttributeDescription>& getAttributeDescriptions() const;
		const Vector<VkVertexInputBindingDescription>& getBindingDescriptions() const;

	private:
		Vector<VkVertexInputAttributeDescription> m_attributes;
		Vector<VkVertexInputBindingDescription> m_bindings;
	};

	// //

	void initVertexTypes();

	struct PrimitiveVertex
	{
		glm::vec3 pos;
	};

	extern VertexFormat g_primitiveVertexFormat;

	struct PrimitiveUVVertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
	};

	extern VertexFormat g_primitiveUvVertexFormat;

	struct ModelVertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 col;
		glm::vec3 norm;
		glm::vec3 tangent;
	};

	extern VertexFormat g_modelVertexFormat;
}

#endif // VERTEX_DESCRIPTOR_H_

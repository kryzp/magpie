#ifndef VERTEX_DESCRIPTOR_H_
#define VERTEX_DESCRIPTOR_H_

#include "../third_party/volk.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "../common.h"
#include "../container/vector.h"

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

	struct ModelVertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 col;
		glm::vec3 norm;
		glm::vec3 tangent;
	};

	void initVertexTypes();

	extern VertexFormat g_modelVertexFormat;
}

#endif // VERTEX_DESCRIPTOR_H_

#pragma once

#include <glm/glm.hpp>

#include "graphics/vertex_format.h"

namespace mgp
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

	namespace vertex_types
	{
		extern VertexFormat PRIMITIVE_VERTEX_FORMAT;
		extern VertexFormat PRIMITIVE_UV_VERTEX_FORMAT;
		extern VertexFormat MODEL_VERTEX_FORMAT;

		void initVertexTypes();
	}
}

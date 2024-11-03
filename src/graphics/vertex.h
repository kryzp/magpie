#ifndef VERTEX_H_
#define VERTEX_H_

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace llt
{
	/**
	* Represents a vertex that can be drawn to the screen.
	*/
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 col;
		glm::vec3 norm;
	};
}

#endif // VERTEX_H_

#ifndef ENTITY_H_
#define ENTITY_H_

#include "math/transform.h"
#include "graphics/mesh.h"

namespace llt
{
	class Entity
	{
	public:
		Entity();
		~Entity();

		void storePrevMatrix();

		const glm::mat4& getPrevMatrix() const;

		Transform transform;
		Mesh* mesh;

	private:
		glm::mat4 m_prevMatrix;
	};
}

#endif // ENTITY_H_

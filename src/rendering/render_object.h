#ifndef ENTITY_H_
#define ENTITY_H_

#include "math/transform.h"
#include "rendering/mesh.h"

namespace llt
{
	class RenderObject
	{
	public:
		RenderObject();
		~RenderObject();

		void storePrevMatrix();

		const glm::mat4& getPrevMatrix() const;

		Transform transform;
		Mesh *mesh;

	private:
		glm::mat4 m_prevMatrix;
	};
}

#endif // ENTITY_H_

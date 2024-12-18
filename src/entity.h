#ifndef ENTITY_H_
#define ENTITY_H_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace llt
{
	class Entity
	{
	public:
		Entity();
		~Entity();

		glm::mat4 getMatrix();
		glm::mat4 getPrevMatrix();

		void storePrevMatrix();

		void setPosition(const glm::vec3& position);
		void setOrigin(const glm::vec3& origin);
		void setRotation(float angle, const glm::vec3& axis);
		void setScale(const glm::vec3& scale);

	private:
		void rebuildMatrix();

		glm::mat4 m_matrix;
		glm::mat4 m_prevMatrix;

		bool m_matrixDirty;

		glm::vec3 m_position;
		glm::vec3 m_origin;
		glm::quat m_rotation;
		glm::vec3 m_scale;
	};
}

#endif // ENTITY_H_

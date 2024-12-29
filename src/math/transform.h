#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace llt
{
	class Transform
	{
	public:
		Transform();
		~Transform();

		glm::mat4 getMatrix();

		void setPosition(const glm::vec3& position);
		void setOrigin(const glm::vec3& origin);
		void setRotation(float angle, const glm::vec3& axis);
		void setScale(const glm::vec3& scale);

	private:
		void rebuildMatrix();

		glm::mat4 m_matrix;

		bool m_matrixDirty;

		glm::vec3 m_position;
		glm::vec3 m_origin;
		glm::quat m_rotation;
		glm::vec3 m_scale;

	};
};

#endif // TRANSFORM_H_

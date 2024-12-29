#include "entity.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace llt;

Entity::Entity()
	: transform()
	, mesh()
	, m_prevMatrix(glm::identity<glm::mat4>())
{
}

Entity::~Entity()
{
}

void Entity::storePrevMatrix()
{
	m_prevMatrix = transform.getMatrix();
}

const glm::mat4& Entity::getPrevMatrix() const
{
	return m_prevMatrix;
}

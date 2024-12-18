#include "entity.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace llt;

Entity::Entity()
	: m_matrix(glm::identity<glm::mat4>())
	, m_prevMatrix(glm::identity<glm::mat4>())
	, m_matrixDirty(false)
	, m_position()
	, m_origin()
	, m_rotation()
	, m_scale()
{
}

Entity::~Entity()
{
}

void Entity::storePrevMatrix()
{
	m_prevMatrix = m_matrix;
}

glm::mat4 Entity::getMatrix()
{
	if (m_matrixDirty) {
		rebuildMatrix();
		m_matrixDirty = false;
	}

	return m_matrix;
}

glm::mat4 Entity::getPrevMatrix()
{
	return m_prevMatrix;
}

void Entity::rebuildMatrix()
{
	m_matrix = glm::identity<glm::mat4>();

	m_matrix = glm::translate(glm::identity<glm::mat4>(), -m_origin) * m_matrix;
	m_matrix = glm::mat4_cast(m_rotation) * m_matrix;
	m_matrix = glm::scale(glm::identity<glm::mat4>(), m_scale) * m_matrix;
	m_matrix = glm::translate(glm::identity<glm::mat4>(), m_position) * m_matrix;
}

void Entity::setPosition(const glm::vec3& position)
{
	m_position = position;
	m_matrixDirty = true;
}

void Entity::setOrigin(const glm::vec3& origin)
{
	m_origin = origin;
	m_matrixDirty = true;
}

void Entity::setRotation(float angle, const glm::vec3& axis)
{
	m_rotation = glm::angleAxis(angle, axis);
	m_matrixDirty = true;
}

void Entity::setScale(const glm::vec3& scale)
{
	m_scale = scale;
	m_matrixDirty = true;
}

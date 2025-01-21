#include "render_object.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace llt;

RenderObject::RenderObject()
	: transform()
	, mesh()
	, m_prevMatrix(glm::identity<glm::mat4>())
{
}

RenderObject::~RenderObject()
{
}

void RenderObject::storePrevMatrix()
{
	m_prevMatrix = transform.getMatrix();
}

const glm::mat4& RenderObject::getPrevMatrix() const
{
	return m_prevMatrix;
}

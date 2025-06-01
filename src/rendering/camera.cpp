#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <glm/vec3.hpp>
#include <glm/glm.hpp>

#include "core/platform.h"

#include "input/input.h"

using namespace mgp;

Camera::Camera(float aspect, float fov, float near, float far)
	: position(0.0f, 0.0f, 0.0f)
	, up(0.0f, 1.0f, 0.0f)
	, direction(0.0f, 0.0f, -1.0f)
	, aspect(aspect)
	, fov(fov)
	, near(near)
	, far(far)
	, m_yaw(0.0f)
	, m_targetYaw(0.0f)
	, m_pitch(0.0f)
	, m_targetPitch(0.0f)
{
}

Camera::~Camera()
{
}

void Camera::update(const InputState *input, const Platform *platform, float dt)
{
	const float MOUSE_DEADZONE = 0.001f;
	const float TURN_SPEED = 1.0f;
	const float MOVE_SPEED = 2.5f;
	
	auto lerp = [&](float from, float to, float t) -> float {
		return from + t*(to - from);
	};

	float dx = (float)(input->getMousePosition().x - platform->getWindowSize().x*0.5f);
	float dy = (float)(input->getMousePosition().y - platform->getWindowSize().y*0.5f);

	if ((dx * dx) + (dy * dy) > MOUSE_DEADZONE*MOUSE_DEADZONE) {
		m_targetYaw -= dx * TURN_SPEED * dt;
		m_targetPitch -= dy * TURN_SPEED * dt;
	}

	m_yaw = lerp(m_yaw, m_targetYaw, dt * 50.0f);
	m_pitch = lerp(m_pitch, m_targetPitch, dt * 50.0f);

	float yaw = m_yaw + glm::half_pi<float>();
	float pitch = m_pitch;

	if (input->isDown(KB_KEY_L))
	{
		position = { -2.0f, 2.0f, 1.0f };
		direction = glm::normalize((glm::vec3) { 1.0f, -1.0f, -1.0f });
	}
	else
	{
		direction.x = glm::cos(yaw) * glm::cos(pitch);
		direction.y = glm::sin(pitch);
		direction.z = -glm::sin(yaw) * glm::cos(pitch);
	}

	glm::vec3 v1 = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 v2 = glm::normalize(glm::cross(v1, direction));

	if (input->isDown(KB_KEY_A)) {
		position += -v1 * MOVE_SPEED * dt;
	} else if (input->isDown(KB_KEY_D)) {
		position += +v1 * MOVE_SPEED * dt;
	}

	if (input->isDown(KB_KEY_W)) {
		position += +direction * MOVE_SPEED * dt;
	} else if (input->isDown(KB_KEY_S)) {
		position += -direction * MOVE_SPEED * dt;
	}

	if (input->shift() || input->ctrl()) {
		position += -v2 * MOVE_SPEED * dt;
	} else if (input->isDown(KB_KEY_SPACE)) {
		position += +v2 * MOVE_SPEED * dt;
	}
}

glm::mat4 Camera::getView() const
{
	return glm::lookAt(position, position + direction, up);
}

glm::mat4 Camera::getRotationMatrix() const
{
	return glm::lookAt(glm::zero<glm::vec3>(), direction, up);
}

glm::mat4 Camera::getProj() const
{
	return glm::perspective(glm::radians(fov), aspect, near, far);
}

void Camera::setYaw(double yaw)
{
	m_yaw = m_targetYaw = yaw;
}

void Camera::setPitch(double pitch)
{
	m_pitch = m_targetPitch = pitch;
}

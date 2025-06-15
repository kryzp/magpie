#pragma once

#include <glm/mat4x4.hpp>

namespace mgp
{
	class InputState;
	class PlatformCore;

	class Camera
	{
	public:
		Camera() = default;
		Camera(float aspect, float fov, float near, float far);

		~Camera() = default;

		void update(const InputState *input, const PlatformCore *platform, float dt);

		glm::mat4 getView() const;
		glm::mat4 getRotationMatrix() const;
		glm::mat4 getProj() const;

		void setYaw(double yaw);
		void setPitch(double pitch);

		glm::vec3 position;
		glm::vec3 up;
		glm::vec3 direction;

		float aspect;
		float fov;
		float near;
		float far;

	private:
		float m_yaw = 0.0f;
		float m_targetYaw = 0.0f;
	
		float m_pitch = 0.0f;
		float m_targetPitch = 0.0f;
	};
}

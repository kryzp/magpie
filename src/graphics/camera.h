#pragma once

#include "math/vec3.h"
#include "math/rect.h"
#include "math/mat4.h"

namespace gfx
{
	struct FrustumVolume {
		Vec4 frustum_planes[6];
	};

	class Camera {
	public:
		enum Type {
			TYPE_PERSPECTIVE,
			TYPE_ORTHOGRAPHIC
		};

		Camera();
		~Camera();

		static Camera perspective(
			const Vec3 &position,
			const Vec3 &forward,
			float fov, float aspect,
			float near, float far
		);

		static Camera orthographic(
			const Vec3 &position,
			const Vec3 &forward,
			const Rect &rect,
			float near, float far
		);

		void recompute();

		FrustumVolume frustum_volume() const;

		const Vec3 &get_position() const
		{
			return position;
		}

		void set_position(const Vec3 &position)
		{
			this->position = position;
		}

		void move_by(const Vec3 &dx)
		{
			this->position += dx;
		}

		const Vec3 &get_forward() const
		{
			return forward;
		}

		void set_forward(const Vec3 &forward)
		{
			this->forward = forward;
		}

		const Vec3 &get_up() const
		{
			return up;
		}

		void set_up(const Vec3 &up)
		{
			this->up = up;
		}

		float get_fov() const
		{
			return fov;
		}

		void set_fov(float fov)
		{
			this->fov = fov;
		}

		float get_aspect() const
		{
			return aspect;
		}

		void set_aspect(float aspect)
		{
			this->aspect = aspect;
		}

		const Rect &get_ortho() const
		{
			return ortho;
		}

		void set_ortho(const Rect &rect)
		{
			this->ortho = rect;
		}

		float get_near() const
		{
			return near_plane;
		}

		void set_near(float near)
		{
			this->near_plane = near;
		}

		float get_far() const
		{
			return far_plane;
		}

		void set_far(float far)
		{
			this->far_plane = far;
		}

		const Mat4 &get_view() const
		{
			return view;
		}

		const Mat4 &get_projection() const
		{
			return proj;
		}

	private:
		Type type;

		Vec3 position;
		Vec3 forward;
		Vec3 up;

		float fov;
		float aspect;

		Rect ortho;

		float near_plane;
		float far_plane;

		Mat4 view;
		Mat4 proj;
	};
}

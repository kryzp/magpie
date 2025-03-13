#ifndef RECT_H_
#define RECT_H_

#include <glm/vec2.hpp>

namespace llt
{
	template <typename T>
	struct Rect
	{
		T x;
		T y;
		T w;
		T h;

		Rect();
		Rect(T wh);
		Rect(T w, T h);
		Rect(T x, T y, T w, T h);

		// enable implicit casting to other rects
		template <typename Y>
		Rect(const Rect<Y> &other) noexcept
			: x(other.x)
			, y(other.y)
			, w(other.w)
			, h(other.h)
		{
		}

		static const Rect &zero();
		static const Rect &one();

		bool contains(const glm::vec<2, T> &other) const;
		bool intersects(const Rect &other) const;

		glm::vec<2, T> position() const;
		glm::vec<2, T> size() const;

		T left() const;
		T right() const;
		T top() const;
		T bottom() const;

		glm::vec<2, T> topLeft() const;
		glm::vec<2, T> topRight() const;
		glm::vec<2, T> bottomLeft() const;
		glm::vec<2, T> bottomRight() const;

		/*
		 * Operator overloading...
		 */
		bool operator == (const Rect &other) const;
		bool operator != (const Rect &other) const;

		Rect operator + (const Rect &other) const;
		Rect operator - (const Rect &other) const;
		Rect operator * (const Rect &other) const;
		Rect operator / (const Rect &other) const;
		
		Rect operator - () const;

		Rect &operator += (const Rect &other);
		Rect &operator -= (const Rect &other);
		Rect &operator *= (const Rect &other);
		Rect &operator /= (const Rect &other);
	};

	using RectF = Rect<float>;
	using RectI = Rect<int>;
	using RectU = Rect<unsigned>;

	template <typename T>
	Rect<T>::Rect()
		: x(0)
		, y(0)
		, w(0)
		, h(0)
	{
	}

	template <typename T>
	Rect<T>::Rect(T wh)
		: x(0)
		, y(0)
		, w(wh)
		, h(wh)
	{
	}

	template <typename T>
	Rect<T>::Rect(T w, T h)
		: x(0)
		, y(0)
		, w(w)
		, h(h)
	{
	}

	template <typename T>
	Rect<T>::Rect(T x, T y, T w, T h)
		: x(x)
		, y(y)
		, w(w)
		, h(h)
	{
	}

	template <typename T>
	bool Rect<T>::contains(const glm::vec<2, T> &other) const
	{
		return (

			// X
			this->left() < other.x &&
			this->right() > other.x &&

			// Y
			this->top() < other.y &&
			this->bottom() > other.y

		);
	}

	template <typename T>
	bool Rect<T>::intersects(const Rect<T> &other) const
	{
		return (

			// X
			this->left() < other.right() &&
			this->right() > other.left() &&

			// Y
			this->top() < other.bottom() &&
			this->bottom() > other.top()

		);
	}

	template <typename T> bool Rect<T>::operator == (const Rect &other) const { return this->x == other.x && this->y == other.y && this->w == other.w && this->h == other.y; }
	template <typename T> bool Rect<T>::operator != (const Rect &other) const { return !(*this == other); }

	template <typename T> Rect<T> Rect<T>::operator + (const Rect &other) const { return Rect(this->x + other.x, this->y + other.y, this->w + other.w, this->h + other.h); }
	template <typename T> Rect<T> Rect<T>::operator - (const Rect &other) const { return Rect(this->x - other.x, this->y - other.y, this->w - other.w, this->h - other.h); }
	template <typename T> Rect<T> Rect<T>::operator * (const Rect &other) const { return Rect(this->x * other.x, this->y * other.y, this->w * other.w, this->h * other.h); }
	template <typename T> Rect<T> Rect<T>::operator / (const Rect &other) const { return Rect(this->x / other.x, this->y / other.y, this->w / other.w, this->h / other.h); }

	template <typename T> Rect<T> Rect<T>::operator - () const { return Rect(-this->x, -this->y, -this->w, -this->h); }

	template <typename T> Rect<T> &Rect<T>::operator += (const Rect &other) { this->x += other.x; this->y += other.y; this->w += other.w; this->h += other.h; return *this; }
	template <typename T> Rect<T> &Rect<T>::operator -= (const Rect &other) { this->x -= other.x; this->y -= other.y; this->w -= other.w; this->h -= other.h; return *this; }
	template <typename T> Rect<T> &Rect<T>::operator *= (const Rect &other) { this->x *= other.x; this->y *= other.y; this->w *= other.w; this->h *= other.h; return *this; }
	template <typename T> Rect<T> &Rect<T>::operator /= (const Rect &other) { this->x /= other.x; this->y /= other.y; this->w /= other.w; this->h /= other.h; return *this; }

	template <typename T> glm::vec<2, T> Rect<T>::position() const { return glm::vec<2, T>(x, y); }
	template <typename T> glm::vec<2, T> Rect<T>::size()     const { return glm::vec<2, T>(w, h); }

	template <typename T> T Rect<T>::left()   const { return x;     }
	template <typename T> T Rect<T>::right()  const { return x + w; }
	template <typename T> T Rect<T>::top()    const { return y;     }
	template <typename T> T Rect<T>::bottom() const { return y + h; }

	template <typename T> glm::vec<2, T> Rect<T>::topLeft()     const { return glm::vec<2, T>(left(),  top());    }
	template <typename T> glm::vec<2, T> Rect<T>::topRight()    const { return glm::vec<2, T>(right(), top());    }
	template <typename T> glm::vec<2, T> Rect<T>::bottomLeft()  const { return glm::vec<2, T>(left(),  bottom()); }
	template <typename T> glm::vec<2, T> Rect<T>::bottomRight() const { return glm::vec<2, T>(right(), bottom()); }

	template <typename T> const Rect<T> &Rect<T>::zero() { static const Rect ZERO = Rect(0, 0, 0, 0); return ZERO; }
	template <typename T> const Rect<T> &Rect<T>::one()  { static const Rect ONE  = Rect(0, 0, 1, 1); return ONE;  }
}

#endif // RECT_H_

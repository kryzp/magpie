#ifndef OPTIONAL_H_
#define OPTIONAL_H_

#include <utility>

namespace llt
{
	/**
	 * Used for when you have a type that you wish to enable
	 * or disable and this cannot be inferred from its current state.
	 */
	template <typename T>
	class Optional
	{
	public:
		Optional();
		Optional(const T& value);
		Optional(const T& value, bool enabled);

		Optional(const Optional& other);
		Optional(Optional&& other) noexcept;

		Optional& operator = (const Optional& other);
		Optional& operator = (Optional&& other) noexcept;

		~Optional();

		T& valueOr(T& other);
		const T& valueOr(const T& other) const;

		T& value();
		const T& value() const;

		constexpr bool hasValue() const;

		void disable();

		explicit operator bool () const;

		bool operator == (const Optional& other) const;
		bool operator != (const Optional& other) const;

	private:
		T m_value;
		bool m_enabled;
	};

	template <typename T>
	Optional<T>::Optional()
		: m_value()
		, m_enabled(false)
	{
	}

	template <typename T>
	Optional<T>::Optional(const T& value)
		: m_value(value)
		, m_enabled(true)
	{
	}

	template <typename T>
	Optional<T>::Optional(const T& value, bool enabled)
		: m_value(value)
		, m_enabled(enabled)
	{
	}

	template <typename T>
	Optional<T>::Optional(const Optional& other)
		: m_value(other.m_value)
		, m_enabled(other.m_enabled)
	{
	}

	template <typename T>
	Optional<T>::Optional(Optional&& other) noexcept
		: m_value(std::move(other.m_value))
		, m_enabled(std::move(other.m_enabled))
	{
		other.m_enabled = false;
	}

	template <typename T>
	Optional<T>& Optional<T>::operator = (const Optional<T>& other)
	{
		this->m_value = other.m_value;
		this->m_enabled = other.m_enabled;

		return *this;
	}

	template <typename T>
	Optional<T>& Optional<T>::operator = (Optional<T>&& other) noexcept
	{
		this->m_value = std::move(other.m_value);
		this->m_enabled = std::move(other.m_enabled);

		other.m_enabled = false;

		return *this;
	}

	template <typename T>
	Optional<T>::~Optional()
	{
		this->m_enabled = false;
	}

	template <typename T>
	T& Optional<T>::valueOr(T& other)
	{
		if (m_enabled) {
			return m_value;
		}

		return other;
	}

	template <typename T>
	const T& Optional<T>::valueOr(const T& other) const
	{
		if (m_enabled) {
			return m_value;
		}

		return other;
	}

	template <typename T>
	T& Optional<T>::value()
	{
		return m_value;
	}

	template <typename T>
	const T& Optional<T>::value() const
	{
		return m_value;
	}

	template <typename T>
	constexpr bool Optional<T>::hasValue() const
	{
		return m_enabled;
	}

	template <typename T>
	void Optional<T>::disable()
	{
		m_enabled = false;
	}

	template <typename T>
	Optional<T>::operator bool() const
	{
		return m_enabled;
	}

	template <typename T>
	bool Optional<T>::operator == (const Optional& other) const
	{
		return this->m_enabled && other.m_enabled && this->m_value == other.m_value;
	}

	template <typename T>
	bool Optional<T>::operator != (const Optional& other) const
	{
		return !((*this) == other);
	}
}

#endif // OPTIONAL_H_

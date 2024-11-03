#ifndef PAIR_H_
#define PAIR_H_

namespace llt
{
	/**
	 * Pair of two different types.
	 */
	template <typename TFirst, typename TSecond>
	struct Pair
	{
		TFirst first;
		TSecond second;

		Pair();
		Pair(const TFirst& a, const TSecond& b);

		Pair(const Pair& other);
		Pair(Pair&& other) noexcept;

		Pair& operator = (const Pair& other);
		Pair& operator = (Pair&& other) noexcept;

		~Pair() = default;

		bool operator == (const Pair& other) const;
		bool operator != (const Pair& other) const;
	};

	template <typename TFirst, typename TSecond>
	Pair<TFirst, TSecond>::Pair()
		: first()
		, second()
	{
	}

	template <typename TFirst, typename TSecond>
	Pair<TFirst, TSecond>::Pair(const TFirst& a, const TSecond& b)
		: first(a)
		, second(b)
	{
	}

	template <typename TFirst, typename TSecond>
	Pair<TFirst, TSecond>::Pair(const Pair& other)
		: first(other.first)
		, second(other.second)
	{
	}

	template <typename TFirst, typename TSecond>
	Pair<TFirst, TSecond>::Pair(Pair&& other) noexcept
		: first(std::move(other.first))
		, second(std::move(other.second))
	{
	}

	template <typename TFirst, typename TSecond>
	Pair<TFirst, TSecond>& Pair<TFirst, TSecond>::operator = (const Pair& other)
	{
		this->first = other.first;
		this->second = other.second;

		return *this;
	}

	template <typename TFirst, typename TSecond>
	Pair<TFirst, TSecond>& Pair<TFirst, TSecond>::operator = (Pair&& other) noexcept
	{
		this->first = std::move(other.first);
		this->second = std::move(other.second);

		return *this;
	}

	template <typename TFirst, typename TSecond>
	bool Pair<TFirst, TSecond>::operator == (const Pair& other) const
	{
		return (
			this->first == other.first &&
			this->second == other.second
		);
	}

	template <typename TFirst, typename TSecond>
	bool Pair<TFirst, TSecond>::operator != (const Pair& other) const
	{
		return !(*this == other);
	}
}

#endif // PAIR_H_

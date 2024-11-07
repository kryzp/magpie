#ifndef BITSET_H_
#define BITSET_H_

#include "../common.h"

namespace llt
{
	/**
	 * A wrapper around a memory efficient array of booleans that
	 * can be toggled isOn and isOff.
	 */
	template <uint64_t Size>
	class Bitset
	{
	public:
		Bitset();
		~Bitset();

		bool all() const;
		bool none() const;
		bool any() const;

		uint64_t onCount() const;
		uint64_t offCount() const;

		Bitset& reset();
		Bitset& invert();

		Bitset& enable(uint64_t idx);
		Bitset& disable(uint64_t idx);
		Bitset& set(uint64_t idx, bool mode);
		Bitset& toggle(uint64_t idx);

		bool isOn(uint64_t idx) const;
		bool isOnOnly(uint64_t idx) const;
		bool isOff(uint64_t idx) const;

		bool operator [] (uint64_t idx) const;
		
		constexpr inline uint64_t size() const;
		constexpr inline uint64_t memorySize() const;

	private:
		byte* m_bytes;
	};

	template <uint64_t Size>
	Bitset<Size>::Bitset()
	{
		m_bytes = new byte[memorySize()];
		reset();
	}

	template <uint64_t Size>
	Bitset<Size>::~Bitset()
	{
		delete[] m_bytes;
	}

	template <uint64_t Size>
	bool Bitset<Size>::all() const
	{
		for (uint64_t i = 0; i < Size; i++)
		{
			if (isOff(i)) {
				return false;
			}
		}

		return true;
	}

	template <uint64_t Size>
	bool Bitset<Size>::none() const
	{
		for (uint64_t i = 0; i < Size; i++)
		{
			if (isOn(i)) {
				return false;
			}
		}

		return true;
	}

	template <uint64_t Size>
	bool Bitset<Size>::any() const
	{
		return !none();
	}

	template <uint64_t Size>
	uint64_t Bitset<Size>::onCount() const
	{
		uint64_t total = 0;
		for (uint64_t i = 0; i < Size; i++)
		{
			if (isOn(i)) {
				total++;
			}
		}

		return total;
	}

	template <uint64_t Size>
	uint64_t Bitset<Size>::offCount() const
	{
		return Size - onCount();
	}

	template <uint64_t Size>
	Bitset<Size>& Bitset<Size>::reset()
	{
		mem::set(m_bytes, 0, memorySize());
		return *this;
	}

	template <uint64_t Size>
	Bitset<Size>& Bitset<Size>::invert()
	{
		for (int i = 0; i < Size; i++) toggle(i);
		return *this;
	}

	template <uint64_t Size>
	Bitset<Size>& Bitset<Size>::enable(uint64_t idx)
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "[BITSET|DEBUG] Index must be within range of the bitset.");
		m_bytes[idx/8] |= (1 << idx);
		return *this;
	}

	template <uint64_t Size>
	Bitset<Size>& Bitset<Size>::disable(uint64_t idx)
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "[BITSET|DEBUG] Index must be within range of the bitset.");
		m_bytes[idx/8] &= ~(1 << idx);
		return *this;
	}

	template <uint64_t Size>
	Bitset<Size>& Bitset<Size>::toggle(uint64_t idx)
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "[BITSET|DEBUG] Index must be within range of the bitset.");
		m_bytes[idx/8] ^= 1 << idx;
		return *this;
	}

	template <uint64_t Size>
	Bitset<Size>& Bitset<Size>::set(uint64_t idx, bool mode)
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "[BITSET|DEBUG] Index must be within range of the bitset.");
		if (mode) m_bytes[idx/8] |= (1 << idx);
		else m_bytes[idx/8] &= ~(1 << idx);
		return *this;
	}

	template <uint64_t Size>
	bool Bitset<Size>::isOn(uint64_t idx) const
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "[BITSET|DEBUG] Index must be within range of the bitset.");
		return (m_bytes[idx/8] & (1 << idx)) != 0;
	}

	template <uint64_t Size>
	bool Bitset<Size>::isOnOnly(uint64_t idx) const
	{
		LLT_ASSERT(idx >= 0 && idx < Size, "[BITSET|DEBUG] Index must be within range of the bitset.");
		return (m_bytes[idx/8] & (1 << idx)) == (1 << idx);
	}

	template <uint64_t Size>
	bool Bitset<Size>::isOff(uint64_t idx) const
	{
		return !isOn(idx);
	}

	template <uint64_t Size>
	bool Bitset<Size>::operator [] (uint64_t idx) const
	{
		return isOn(idx);
	}

	template <uint64_t Size>
	constexpr inline uint64_t Bitset<Size>::size() const
	{
		return Size;
	}

	template <uint64_t Size>
	constexpr inline uint64_t Bitset<Size>::memorySize() const
	{
		return (Size / 8) + 1;
	}
}

#endif // BITSET_H_

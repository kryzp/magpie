#ifndef FUNCTION_H_
#define FUNCTION_H_

#include <memory>

#include "core/common.h"

namespace llt
{
	template <typename T>
	class Function;

	/**
	 * Wrapper around a function pointer pretty much
	 */
	template <typename Result, typename... Args>
	class Function<Result(Args...)>
	{
		typedef Result (*call_fn)(byte*, Args&&...);
		typedef void   (*ctor_fn)(byte*, byte *);
		typedef void   (*dtor_fn)(byte *);

		template <typename F>
		static Result getCallFn(F *fn, Args&&... args)
		{
			return (*fn)(std::forward<Args>(args)...);
		}

		template <typename F>
		static void getCtorFn(F *dst, F *src)
		{
			new (dst) F(*src);
		}

		template <typename F>
		static void getDtorFn(F *fn)
		{
			fn->~F();
		}

	public:
		Function();
		Function(std::nullptr_t null);
		Function(const Function &other);

		template <typename F>
		Function(F fn);

		~Function();

		Result call(Args... args) const;
		Result operator ()(Args... args) const;

		operator bool () const;

		bool operator == (const Function &other);
		bool operator != (const Function &other);

	private:
		call_fn m_callFn;
		ctor_fn m_createFn;
		dtor_fn m_destroyFn;

		byte *m_data;
		uint64_t m_dataSize;
	};

	template <typename Result, typename... Args>
	Function<Result(Args...)>::Function()
		: m_callFn(nullptr)
		, m_createFn(nullptr)
		, m_destroyFn(nullptr)
		, m_data(nullptr)
		, m_dataSize(0)
	{
	}

	template <typename Result, typename... Args>
	Function<Result(Args...)>::Function(std::nullptr_t null)
		: m_callFn(nullptr)
		, m_createFn(nullptr)
		, m_destroyFn(nullptr)
		, m_data(nullptr)
		, m_dataSize(0)
	{
	}

	template <typename Result, typename... Args>
	Function<Result(Args...)>::Function(const Function<Result(Args...)>& other)
		: m_callFn(other.m_callFn)
		, m_createFn(other.m_createFn)
		, m_destroyFn(other.m_destroyFn)
		, m_data(nullptr)
		, m_dataSize(other.m_dataSize)
	{
		if (m_callFn && other.m_data)
		{
			m_data = new byte[this->m_dataSize];
			m_createFn(m_data, other.m_data);
		}
	}

	template <typename Result, typename... Args>
	template <typename F>
	Function<Result(Args...)>::Function(F fn)
		: m_callFn   (reinterpret_cast<call_fn>(getCallFn<F>))
		, m_createFn (reinterpret_cast<ctor_fn>(getCtorFn<F>))
		, m_destroyFn(reinterpret_cast<dtor_fn>(getDtorFn<F>))
	{
		// allocate data for functions
		m_dataSize = sizeof(F);
		m_data = new byte[m_dataSize];
		m_createFn(m_data, reinterpret_cast<byte*>(&fn));
	}

	template <typename Result, typename... Args>
	Function<Result(Args...)>::~Function()
	{
		// make sure to destroy the data
		if (m_destroyFn && m_data) {
			m_destroyFn(m_data);
		}

		delete[] m_data;
	}

	template <typename Result, typename... Args>
	Result Function<Result(Args...)>::call(Args... args) const
	{
		return m_callFn(m_data, std::forward<Args>(args)...);
	}

	template <typename Result, typename... Args>
	Result Function<Result(Args...)>::operator ()(Args... args) const
	{
		return m_callFn(m_data, std::forward<Args>(args)...);
	}

	template <typename Result, typename... Args>
	Function<Result(Args...)>::operator bool () const
	{
		return m_callFn != nullptr;
	}

	template <typename Result, typename... Args>
	bool Function<Result(Args...)>::operator == (const Function &other)
	{
		return (
			this->m_data == other.m_data &&
			this->m_dataSize == other.m_dataSize &&
			this->m_callFn == other.m_callFn &&
			this->m_createFn == other.m_createFn &&
			this->m_destroyFn == other.m_destroyFn
		);
	}

	template <typename Result, typename... Args>
	bool Function<Result(Args...)>::operator != (const Function &other)
	{
		return !(*this == other);
	}
}

#endif // FUNCTION_H_

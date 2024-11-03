#ifndef FUNCTION_H_
#define FUNCTION_H_

#include <memory>
#include "../common.h"

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
		typedef void   (*ctor_fn)(byte*, byte*);
		typedef void   (*dtor_fn)(byte*);

		template <typename F>
		static Result get_call_fn(F* fn, Args&&... args)
		{
			return (*fn)(std::forward<Args>(args)...);
		}

		template <typename F>
		static void get_ctor_fn(F* dst, F* src)
		{
			new (dst) F(*src);
		}

		template <typename F>
		static void get_dtor_fn(F* fn)
		{
			fn->~F();
		}

	public:
		Function();
		Function(std::nullptr_t null);
		Function(const Function& other);

		template <typename F>
		Function(F fn);

		~Function();

		Result call(Args... args) const;
		Result operator ()(Args... args) const;

		operator bool () const;

		bool operator == (const Function& other);
		bool operator != (const Function& other);

	private:
		call_fn m_call_func;
		ctor_fn m_create_func;
		dtor_fn m_destroy_func;

		byte* m_data;
		uint64_t m_data_size;
	};

	template <typename Result, typename... Args>
	Function<Result(Args...)>::Function()
		: m_call_func(nullptr)
		, m_create_func(nullptr)
		, m_destroy_func(nullptr)
		, m_data(nullptr)
		, m_data_size(0)
	{
	}

	template <typename Result, typename... Args>
	Function<Result(Args...)>::Function(std::nullptr_t null)
		: m_call_func(nullptr)
		, m_create_func(nullptr)
		, m_destroy_func(nullptr)
		, m_data(nullptr)
		, m_data_size(0)
	{
	}

	template <typename Result, typename... Args>
	Function<Result(Args...)>::Function(const Function<Result(Args...)>& other)
		: m_call_func(other.m_call_func)
		, m_create_func(other.m_create_func)
		, m_destroy_func(other.m_destroy_func)
		, m_data_size(other.m_data_size)
		, m_data(nullptr)
	{
		if (m_call_func && other.m_data)
		{
			m_data = new byte[this->m_data_size];
			m_create_func(m_data, other.m_data);
		}
	}

	template <typename Result, typename... Args>
	template <typename F>
	Function<Result(Args...)>::Function(F fn)
		: m_call_func   (reinterpret_cast<call_fn>(get_call_fn<F>))
		, m_create_func (reinterpret_cast<ctor_fn>(get_ctor_fn<F>))
		, m_destroy_func(reinterpret_cast<dtor_fn>(get_dtor_fn<F>))
	{
		// allocate data for functions
		m_data_size = sizeof(F);
		m_data = new byte[m_data_size];
		m_create_func(m_data, reinterpret_cast<byte*>(&fn));
	}

	template <typename Result, typename... Args>
	Function<Result(Args...)>::~Function()
	{
		// make sure to destroy the data
		if (m_destroy_func && m_data) {
			m_destroy_func(m_data);
		}

		delete[] m_data;
	}

	template <typename Result, typename... Args>
	Result Function<Result(Args...)>::call(Args... args) const
	{
		return m_call_func(m_data, std::forward<Args>(args)...);
	}

	template <typename Result, typename... Args>
	Result Function<Result(Args...)>::operator ()(Args... args) const
	{
		return m_call_func(m_data, std::forward<Args>(args)...);
	}

	template <typename Result, typename... Args>
	Function<Result(Args...)>::operator bool () const
	{
		return m_call_func != nullptr;
	}

	template <typename Result, typename... Args>
	bool Function<Result(Args...)>::operator == (const Function& other)
	{
		return (
			this->m_data == other.m_data &&
			this->m_data_size == other.m_data_size &&
			this->m_call_func == other.m_call_func &&
			this->m_create_func == other.m_create_func &&
			this->m_destroy_func == other.m_destroy_func
		);
	}

	template <typename Result, typename... Args>
	bool Function<Result(Args...)>::operator != (const Function& other)
	{
		return !(*this == other);
	}
}

#endif // FUNCTION_H_

/*
#pragma once

#include <string>
#include <functional>
#include <unordered_map>

namespace mgp
{
	enum CVarFlags
	{
		CVAR_FLAG_NONE = 0,

		CVAR_FLAG_EDIT_CHECKBOX_BIT 		= 1 << 0,
		CVAR_FLAG_EDIT_INPUT_BIT			= 1 << 1
	};

	template <typename T>
	class CVar
	{
	public:
		CVar(
			const std::string &name,
			const std::string &description,
			const T &defaultValue,
			CVarFlags flags,
			const std::function<void(CVar *, const T &)> &callback
		);

		const std::string &getName() const
		{
			return m_name;
		}

		const std::string &getDescription() const
		{
			return m_description;
		}

		CVarFlags getFlags() const
		{
			return m_flags;
		}

		const T &getValue() const
		{
			return m_value;
		}

		void setValue(const T &value)
		{
			T old = m_value;
			m_value = value;
			m_callback(this, old);
		}

		void imDisplay();

	private:
		std::string m_name;
		std::string m_description;

		CVarFlags m_flags;

		const std::function<void(CVar *, const T &)> m_callback;

		const T m_defaultValue;

		T m_value;
	};

	class CVarSystem
	{
		template <typename T>
		struct CVarList
		{
			std::unordered_map<std::string, CVar<T> *> cvars;

			void add(CVar<T> *cvar)
			{
				cvars.insert({ cvar->getName(), cvar });
			}

			CVar<T> *get(const std::string &name)
			{
				return cvars.at(name);
			}
		};

	public:
		template <typename T>
		static void registerCVar(CVar<T> *cvar)
		{
			getCVarList<T>().add(cvar);
		}

		template <typename T>
		static CVar<T> *getCVar(const std::string &name)
		{
			return getCVarList<T>().get(name);
		}

	private:
		template <typename T>
		static CVarList<T> &getCVarList();

		template <> CVarList<int> &getCVarList() { return m_cvarListInt; }
		template <> CVarList<float> &getCVarList() { return m_cvarListFloat; }

		static CVarList<int> m_cvarListInt;
		static CVarList<float> m_cvarListFloat;
	};

	template <typename T>
	CVar<T>::CVar(
		const std::string &name,
		const std::string &description,
		const T &defaultValue,
		CVarFlags flags,
		const std::function<void(CVar *, const T &)> &callback
	)
		: m_name(name)
		, m_description(description)
		, m_flags(flags)
		, m_callback(callback)
		, m_defaultValue(defaultValue)
		, m_value(defaultValue)
	{
		CVarSystem::registerCVar<T>(this);
	}
}
*/

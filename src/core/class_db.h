#pragma once

// Implementation inspired by Source engine.

#include <atomic>
#include <type_traits>

#include "types.h"
#include "hash.h"

#include "container/hash_map.h"

enum FieldType {
	FIELD_INT,
	FIELD_FLOAT,
	FIELD_BOOL,
	FIELD_STRING,
	FIELD_OBJECT,
	FIELD_MAX_ENUM
};

struct FieldInfo {
	const char *name;
	FieldType type;
	u64 offset;
	u64 target_type_id; // For non-primitive field types.
	bool is_pointer;
};

class Object;

struct TypeInfo {
	const char *name;
	const TypeInfo *parent;

	u64 type_id;

	Object *(*factory)(void);

	u32 field_count;
	const FieldInfo *fields;

	bool is_derived_from(const TypeInfo *other) const
	{
		const TypeInfo *curr = this;
		while (curr) {
			if (curr == other)
				return true;
			curr = curr->parent;
		}
		return false;
	}
};

namespace class_db_static_init
{
	struct TypeRegistrationNode {
		const TypeInfo *info;
		TypeRegistrationNode *next;
	};

	extern std::atomic<TypeRegistrationNode *> registration_list_head;

	inline void register_class(TypeRegistrationNode *node)
	{
		node->next = registration_list_head.load(std::memory_order_relaxed);

		// This literally should never happen but just in-case
		// there is a thread collision during boot... somehow.
		while (!registration_list_head.compare_exchange_weak(
			node->next, node,
			std::memory_order_release,
			std::memory_order_relaxed
		)) { }
	}
}

class ClassDB {
public:
	ClassDB() = default;
	~ClassDB() = default;

	static ClassDB *get_singleton();

	void build_registry();
	
	Object *instantiate(u64 type_id);

	const TypeInfo *get_type(const char *name) const;
	const TypeInfo *get_type_by_id(u64 type_id) const;

private:
	HashMap<u64, const TypeInfo *> registry;
};

class Object {
public:
	virtual ~Object() = default;

	static const TypeInfo *get_class_static()
	{
		// Lambda trick to make this thread safe (C++ static lock)
		static TypeInfo info = {};
		static class_db_static_init::TypeRegistrationNode node = {};

		static bool init = []() {
			info.name = "Object";
			info.parent = nullptr;
			info.type_id = hash::cstr(info.name);
			info.field_count = 0;
			info.fields = nullptr;

			node.info = &info;
			class_db_static_init::register_class(&node);
			
			return true;
		}();

		return &info;
	}

	virtual const TypeInfo *get_class_info() const
	{
		return get_class_static();
	}

	template <typename T>
	bool is_class() const
	{
		return get_class_info()->is_derived_from(T::get_class_static());
	}

	template <typename T>
	T *try_cast_to()
	{
		return is_class<T>() ? static_cast<T *>(this) : nullptr;
	}

	template <typename T>
	const T *try_cast_to() const
	{
		return is_class<T>() ? static_cast<const T *>(this) : nullptr;
	}
};

#define DB_DATA_DECLARE(class_, inherits_)						\
public:															\
	static const TypeInfo *get_class_static();					\
	virtual const TypeInfo *get_class_info() const override		\
	{															\
		return get_class_static();								\
	}															\
private:														\
	typedef inherits_ BaseClass;								\
	typedef class_ ThisClass

#define DB_DATA_BEGIN(class_)									\
	struct class_##_TYPE_REG {									\
		class_##_TYPE_REG() { class_::get_class_static(); }		\
	};															\
	static class_##_TYPE_REG internal_##class_##_type_init;		\
	const TypeInfo *class_::get_class_static()					\
	{															\
		static TypeInfo info = {};								\
		static class_db_static_init::TypeRegistrationNode node = {}; \
		static bool init = []() {								\
			info.name = #class_;								\
			info.parent = BaseClass::get_class_static();		\
			info.type_id = hash::cstr(#class_);					\
			info.factory = []() -> Object * { return new class_(); }; \
			static FieldInfo fields[] = {

#define DB_DATA_FIELD(name_, type_)								\
				{ #name_, type_, offsetof(ThisClass, name_), 0, std::is_pointer<decltype(ThisClass::name_)>::value },

#define DB_DATA_OBJECT(name_, type_, target_class_)				\
				{ #name_, type_, offsetof(ThisClass, name_), hash::cstr(#target_class_), std::is_pointer<decltype(ThisClass::name_)>::value },

#define DB_DATA_END()											\
			};													\
			info.field_count = array_size(fields);				\
			info.fields = fields;								\
			node.info = &info;									\
			class_db_static_init::register_class(&node);		\
			return true;										\
		}();													\
		return &info;											\
	}

#define DB_DATA_EMPTY(class_)									\
	struct class_##_TYPE_REG {									\
		class_##_TYPE_REG() { class_::get_class_static(); }		\
	};															\
	static class_##_TYPE_REG internal_##class_##_type_init;		\
	const TypeInfo *class_::get_class_static()					\
	{															\
		static TypeInfo info = {};								\
		static class_db_static_init::TypeRegistrationNode node = {}; \
		static bool init = []() {								\
			info.name = #class_;								\
			info.parent = BaseClass::get_class_static();		\
			info.type_id = hash::cstr(#class_);					\
			info.factory = []() -> Object * { return new class_(); }; \
			info.field_count = 0;								\
			info.fields = nullptr;								\
			node.info = &info;									\
			class_db_static_init::register_class(&node);		\
			return true;										\
		}();													\
		return &info;											\
	}

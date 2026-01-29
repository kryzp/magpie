#pragma once

#include "types.h"
#include "hash.h"

#include "container/hash_map.h"

enum FieldType {
	FIELD_INT,
	FIELD_FLOAT,
	FIELD_BOOL,
	FIELD_STRING,
	FIELD_MAX_ENUM
};

struct FieldInfo {
	const char *name;
	FieldType type;
	u64 offset;
};

struct TypeInfo {
	const char *name;
	const TypeInfo *parent;

	u64 type_id;

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

class Object {
public:
	virtual ~Object() = default;
	static const TypeInfo *get_class_static();
	virtual const TypeInfo *get_class_info() const;

	template <typename T>
	bool is_class() const
	{
		return get_class_info()->is_derived_from(T::get_class_static());
	}

	template <typename T>
	T *try_cast_to()
	{
		return is_class<T>() ? (T *)this : nullptr;
	}
};

#define DECLARE_CLASS(class_, inherits_)						\
	typedef inherits_ BaseClass;								\
	typedef class_ ThisClass;									\

#define DB_DATA_DECLARE(class_)									\
public:															\
	static const TypeInfo *get_class_static();					\
	virtual const TypeInfo *get_class_info() const override		\
	{															\
		return get_class_static();								\
	}															\
private:

#define DB_DATA_BEGIN(class_)									\
	struct class_##_TYPE_REG {									\
		class_##_TYPE_REG() { class_::get_class_static(); }		\
	};															\
	static class_##_TYPE_REG internal_##class_##_type_init;		\
	const TypeInfo *class_::get_class_static()					\
	{															\
		static TypeInfo info = []() {							\
			TypeInfo i = {};									\
			i.name = #class_;									\
			i.parent = BaseClass::get_class_static();			\
			i.type_id = hash::cstr(#class_);					\
			static FieldInfo fields[] = {

#define DB_DATA_FIELD(name_, type_)								\
				{ #name_, type_, offsetof(ThisClass, name_) },

#define DB_DATA_END()											\
			};													\
			i.field_count = array_size(fields);					\
			i.fields = fields;									\
			ClassDB::get_singleton()->register_class(&i);		\
			return i;											\
		}();													\
		return &info;											\
	}

class ClassDB {
public:
	ClassDB() = default;
	~ClassDB() = default;

	static ClassDB *get_singleton();

	void register_class(const TypeInfo *info);

	const TypeInfo *get_type(const char *name) const;
	const TypeInfo *get_type_by_id(u64 type_id) const;

private:
	HashMap<u64, const TypeInfo *> registry;
};

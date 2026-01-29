#include "class_db.h"

const TypeInfo *Object::get_class_static()
{
	// Lambda trick to make this thread safe (C++ static lock)
	static TypeInfo info = []() {
		TypeInfo i = {};
		i.name = "Object";
		i.parent = nullptr;
		i.type_id = hash::cstr(i.name);
		i.field_count = 0;
		i.fields = nullptr;
		return i;
	}();

	return &info;
}

const TypeInfo *Object::get_class_info() const
{
	return get_class_static();
}

ClassDB *ClassDB::get_singleton()
{
	static ClassDB instance;
	return &instance;
}

void ClassDB::register_class(const TypeInfo *info)
{
	registry[info->type_id] = info;
}

const TypeInfo *ClassDB::get_type(const char *name) const
{
	return get_type_by_id(hash::cstr(name));
}

const TypeInfo *ClassDB::get_type_by_id(u64 type_id) const
{
	auto it = registry.find(type_id);
	if (it == registry.end())
		return nullptr;
	return it->second;
}

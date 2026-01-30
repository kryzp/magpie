#include "class_db.h"

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

#include "class_db.h"

namespace class_db_static_init
{
	std::atomic<TypeRegistrationNode *> registration_list_head{nullptr};
}

ClassDB *ClassDB::get_singleton()
{
	static ClassDB instance;
	return &instance;
}

void ClassDB::build_registry()
{
	class_db_static_init::TypeRegistrationNode *curr = class_db_static_init::registration_list_head.load(std::memory_order_acquire);

	while (curr) {
		registry[curr->info->type_id] = curr->info;
		curr = curr->next;
	}
}

Object *ClassDB::instantiate(u64 type_id)
{
	auto it = registry.find(type_id);
	if (it != registry.end() && it->second->factory)
		return it->second->factory();
	return nullptr;
}

const TypeInfo *ClassDB::get_type(const char *name) const
{
	return get_type_by_id(hash::cstr(name));
}

const TypeInfo *ClassDB::get_type_by_id(u64 type_id) const
{
	auto it = registry.find(type_id);
	if (it != registry.end())
		return it->second;
	return nullptr;
}

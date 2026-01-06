
bool entity_handle_valid(struct entity_world *world, struct entity_handle handle)
{
	return (handle.type != ENTITY_TYPE_max_enum &&
		handle.index != ENTITY_HANDLE_INVALID_INDEX &&
		handle.index < ENTITY_WORLD_MAX_ENTITIES &&
		handle.generation == world->generations[handle.type][handle.index]);
}

bool entity_handle_equal(struct entity_handle a, struct entity_handle b)
{
	return (a.type == b.type &&
		a.index == b.index &&
		a.generation == b.generation);
}

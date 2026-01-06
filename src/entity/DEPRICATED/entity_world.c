
void entity_world_init(struct entity_world *world, struct memory_arena *arena)
{
	world->arena = arena;
	entity_signal_dispatcher_init(&world->dispatcher, world->arena, 256);
}

static void _entity_world_unload_scene(struct entity_world *world, struct entity_scene *scene)
{
	for (u32 i = 0; i < scene->entity_count; i++) {
		struct entity_scene_map_entity *map_entity = scene->entities + i;
		for (u32 j = 0; j < world->entity_count[map_entity->type]; j++) {
			void *entity = world->entities[map_entity->type][j];
			struct entity_shared_data *data = entity_get_shared_data(entity);
			if (data->current_scene_id == scene->scene_id && // Only unload entities that are still IN that scene.
			    data->original_map_id == map_entity->map_id)
				entity_world_destroy_entity(world, data->handle);
		}
	}
}

static void _entity_world_load_scene(struct entity_world *world, struct entity_scene *scene)
{
	for (u32 k = 0; k < scene->entity_count; k++) {
		struct entity_scene_map_entity *e = scene->entities + k;
		enum entity_scene_context_type t = ENTITY_SCENE_CONTEXT_TYPE_max_enum;
		for (u32 i = 0; i < world->scene_context_count; i++) {
			struct entity_scene_context *c = world->scene_contexts + i;
			if (c->entity.scene_id == scene->scene_id &&
			    c->entity.map_id == e->map_id) {
				t = c->type;
				break;
			}
		}
		switch (t) {
		case ENTITY_SCENE_CONTEXT_TYPE_skip_entity:
			break;
		default: {
			struct entity_handle handle = entity_world_add_entity(world, e->type, e->transform, e->settings);
			void *entity = entity_world_get_entity(world, handle);
			struct entity_shared_data *data = entity_get_shared_data(entity);
			data->original_map_id = e->map_id;
			data->original_scene_id = scene->scene_id;
			data->current_scene_id = data->original_scene_id;
			break;
		}
		}
	}
}

void entity_world_load_scene(struct entity_world *world, struct entity_scene *scene)
{
	struct scratch_arena scratch = scratch_begin(world->arena, 1);
	
	u32 neighbour_count = 0;
	struct entity_scene *neighbours = entity_scene_get_neighbouring_scenes(scene, scratch.arena, &neighbour_count);

	for (u32 i = 0; i < neighbour_count; i++) {
		struct entity_scene *s = neighbours + i;
		bool do_load = true;
		for (u32 j = 0; j < world->active_scene_count; j++) {
			if (world->active_scenes[j] == s) {
				do_load = false;
				break;
			}
		}
		if (do_load)
			_entity_world_load_scene(world, s);
		
		world->active_scenes[i] = s;
	}

	_entity_world_load_scene(world, scene);

	world->active_scenes[neighbour_count] = scene;
	world->active_scene_count = neighbour_count + 1;
	
	entity_world_resolve_destroying(world);

	scratch_release(&scratch);
}

void entity_world_push_scene_context(struct entity_world *world, struct entity_shared_data *data, enum entity_scene_context_type type)
{
	struct entity_scene_context *c = world->scene_contexts + world->scene_context_count;
	c->entity.scene_id = data->original_scene_id;
	c->entity.map_id = data->original_map_id;
	c->type = type;
	
	world->scene_context_count++;
}

static struct entity_handle _entity_world_create_handle(struct entity_world *world, enum entity_type type, void *entity)
{
	struct entity_handle handle = {0};
	handle.type = type;

	handle.index = world->free_slot_count[type] > 0
		? world->free_slots[type][--world->free_slot_count[type]]
		: world->entity_count[type]++;

	handle.generation = world->generations[type][handle.index];
	
	world->entities[type][handle.index] = entity;
	return handle;
}

struct entity_handle entity_world_add_entity(struct entity_world *world,
					     enum entity_type type,
					     struct transform transform,
					     void *settings)
{
	struct entity_shared_data *data = NULL;
	switch (type) {
#define ENTITY_DEF(__type)						\
		case ENTITY_TYPE_##__type: {				\
			struct __type *entity = (struct __type *)memory_arena_push(world->arena, sizeof(struct __type)); \
			data = entity_get_shared_data(entity);		\
			data->type = type;				\
			data->handle = _entity_world_create_handle(world, type, entity); \
			entity_##__type##_init(entity, world, transform, settings); \
			break;						\
		}
#include "entities.inc"
#undef ENTITY_DEF
	}
	return data->handle;
}

bool entity_world_destroy_entity(struct entity_world *world, struct entity_handle handle)
{
	if (!entity_handle_valid(world, handle))
		return false;
	world->destroying[world->destroying_count++] = handle;
	return true;
}

void entity_world_resolve_destroying(struct entity_world *world)
{
	for (u32 i = 0; i < world->destroying_count; i++) {
		struct entity_handle *handle = world->destroying + i;
		
		switch (handle->type) {
#define ENTITY_DEF(__type)						\
			case ENTITY_TYPE_##__type: {			\
				void *entity = world->entities[handle->type][handle->index]; \
				entity_##__type##_destroy((struct __type *)entity, world); \
				break;					\
			}
#include "entities.inc"
#undef ENTITY_DEF
		}
		
		world->entities[handle->type][handle->index] = NULL;
		world->generations[handle->type][handle->index]++;
		
		world->free_slots[handle->type][world->free_slot_count[handle->type]++] = handle->index;
	}
	world->destroying_count = 0;
}

void entity_world_tick(struct entity_world *world, float dt)
{
	for (u32 t = 0; t < ENTITY_TYPE_max_enum; t++) {
		for (u32 i = 0; i < world->entity_count[t]; i++) {
			void *entity = world->entities[t][i];
			if (!entity)
				continue;
			switch (t) {
#define ENTITY_DEF(__type)						\
				case ENTITY_TYPE_##__type:		\
					entity_##__type##_tick((struct __type *)entity, world, dt); \
					break;
#include "entities.inc"
#undef ENTITY_DEF
			}
		}
	}
}

void entity_world_tick_fixed(struct entity_world *world, float dt)
{
	for (u32 t = 0; t < ENTITY_TYPE_max_enum; t++) {
		for (u32 i = 0; i < world->entity_count[t]; i++) {
			void *entity = world->entities[t][i];
			if (!entity)
				continue;
			switch (t) {
#define ENTITY_DEF(__type)						\
				case ENTITY_TYPE_##__type:		\
					entity_##__type##_tick_fixed((struct __type *)entity, world, dt); \
					break;
#include "entities.inc"
#undef ENTITY_DEF
			}
		}
	}
}

void *entity_world_get_entity(struct entity_world *world, struct entity_handle handle)
{
	if (!entity_handle_valid(world, handle))
		return NULL;

	if (handle.index >= ENTITY_WORLD_MAX_ENTITIES)
		return NULL;

	if (world->generations[handle.type][handle.index] != handle.generation)
		return NULL;

	return world->entities[handle.type][handle.index];
}

void entity_world_for_each(struct entity_world *world, entity_world_for_each_fn_t fn, void *context)
{
	for (u32 t = 0; t < ENTITY_TYPE_max_enum; t++) {
		for (u32 i = 0; i < world->entity_count[t]; i++) {
			void *entity = world->entities[t][i];
			if (entity && !fn(entity, context))
				return;
		}
	}
}

struct entity_scene *entity_world_get_main_scene(struct entity_world *world)
{
	return world->active_scenes[world->active_scene_count - 1];
}

struct entity_list entity_world_query(struct entity_world *world, struct memory_arena *arena, bool (*query)(void *))
{
	struct entity_list list = entity_list_init(arena);
	for (u32 t = 0; t < ENTITY_TYPE_max_enum; t++) {
		for (u32 i = 0; i < world->entity_count[t]; i++) {
			void *entity = world->entities[t][i];
			if (entity && query(entity))
				entity_list_add(&list, entity);
		}
	}
	return list;
}

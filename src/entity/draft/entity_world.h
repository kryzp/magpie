
enum entity_scene_context_type {
	ENTITY_SCENE_CONTEXT_TYPE_skip_entity,
	ENTITY_SCENE_CONTEXT_TYPE_max_enum
};

struct entity_scene_context {
	struct entity_scene_map_entity_handle entity;
	enum entity_scene_context_type type;
};

#define ENTITY_WORLD_MAX_ENTITIES 256
#define ENTITY_WORLD_MAX_SCENE_CONTEXTS 64
#define ENTITY_WORLD_MAX_FREE_HANDLES 16
#define ENTITY_WORLD_MAX_DESTROYING_ENTITIES 64
#define ENTITY_WORLD_MAX_ACTIVE_SCENES 8

struct entity_world {
	struct memory_arena *arena;

	u32 entity_count[ENTITY_TYPE_max_enum];
	void *entities[ENTITY_TYPE_max_enum][ENTITY_WORLD_MAX_ENTITIES];
	
	u32 generations[ENTITY_TYPE_max_enum][ENTITY_WORLD_MAX_ENTITIES];

	u32 free_slot_count[ENTITY_TYPE_max_enum];
	u32 free_slots[ENTITY_TYPE_max_enum][ENTITY_WORLD_MAX_FREE_HANDLES];

	u32 destroying_count;
	struct entity_handle destroying[ENTITY_WORLD_MAX_DESTROYING_ENTITIES];

	u32 active_scene_count;
	struct entity_scene *active_scenes[ENTITY_WORLD_MAX_ACTIVE_SCENES];
	
	u32 scene_context_count;
	struct entity_scene_context scene_contexts[ENTITY_WORLD_MAX_SCENE_CONTEXTS];

	struct entity_signal_dispatcher dispatcher;
};

void entity_world_init(struct entity_world *world, struct memory_arena *arena);

void entity_world_load_scene(struct entity_world *world, struct entity_scene *scene);

void entity_world_push_scene_context(struct entity_world *world, struct entity_shared_data *data, enum entity_scene_context_type type);

struct entity_handle entity_world_add_entity(struct entity_world *world, enum entity_type type, struct transform transform, void *settings);
b32 entity_world_destroy_entity(struct entity_world *world, struct entity_handle handle);

void entity_world_resolve_destroying(struct entity_world *world);

void entity_world_tick(struct entity_world *world, float dt);
void entity_world_tick_fixed(struct entity_world *world, float dt);

struct entity_handle entity_world_get_free_handle(struct entity_world *world, enum entity_type type);
void *entity_world_get_entity(struct entity_world *world, struct entity_handle handle);

typedef bool (*entity_world_for_each_fn_t)(void *entity, void *context);
void entity_world_for_each(struct entity_world *world, entity_world_for_each_fn_t fn, void *context);

struct entity_scene *entity_world_get_main_scene(struct entity_world *world);

struct entity_list entity_world_query(struct entity_world *world, struct memory_arena *arena, bool (*query)(void *))


struct entity_scene_queue {
	u32 asdf;
};

struct entity_scene_reference {
	u32 scene_id;
	u32 map_id;
};

struct entity_scene_map_entity {
	u32 map_id;
	enum entity_type type;
	struct transform transform;
	void *settings;
};

#define ENTITY_SCENE_MAX_ENTITIES 64

struct entity_scene {
	u32 scene_id;
	u32 entity_count;
	struct entity_scene_map_entity entities[ENTITY_SCENE_MAX_ENTITIES];
};

struct entity_scene *entity_scene_get_neighbouring_scenes(struct entity_scene *scene, struct memory_arena *arena, u32 *count);

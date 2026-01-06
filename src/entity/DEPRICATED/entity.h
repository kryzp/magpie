
struct entity_world;
struct entity_scene;

enum entity_type {
#define ENTITY_DEF(__type) ENTITY_TYPE_##__type,
#include "entities.inc"
#undef ENTITY_DEF
	ENTITY_TYPE_max_enum
};

#define ENTITY_HANDLE_INVALID_INDEX ((u32)(-1))

struct entity_handle {
	enum entity_type type;
	u32 index;
	u32 generation;
};

struct entity_shared_data {
	enum entity_type type;
	u32 original_map_id;
	u32 original_scene_id;
	u32 current_scene_id;
	struct entity_handle handle;
};

#define entity_get_shared_data(entity) ((struct entity_shared_data *)(entity))
#define entity_get_handle(entity) (entity_get_shared_data(entity)->handle)

// Must be defined on the very top of the entity struct.
#define ENTITY_SHARED_DATA struct entity_shared_data _internal_entity_data

bool entity_handle_valid(struct entity_world *world, struct entity_handle handle);
bool entity_handle_equal(struct entity_handle a, struct entity_handle b);

#define ENTITY_DEF(__type)						\
	struct __type;							\
	void entity_##__type##_init        (struct __type *, struct entity_world *, struct transform transform, void *settings); \
	void entity_##__type##_destroy     (struct __type *, struct entity_world *); \
	void entity_##__type##_tick        (struct __type *, struct entity_world *, float dt); \
	void entity_##__type##_tick_fixed  (struct __type *, struct entity_world *, float dt); \
	void entity_##__type##_load        (struct __type *, struct binary_reader *); \
	void entity_##__type##_serialize   (struct __type *, struct binary_writer *);
#include "entities.inc"
#undef ENTITY_DEF

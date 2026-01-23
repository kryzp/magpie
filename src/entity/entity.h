#pragma once

#include "core/types.h"

namespace ent
{
	enum EntityType {
		ENTITY_TYPE_MAX_ENUM
	};

	struct EntityHandle {
		constexpr static u32 INVALID_INDEX = (u32)(-1);

		EntityType type;
		u32 index;
		u32 generation;

		bool is_valid() const;
	};

	class Entity {
	public:
		Entity();
		virtual ~Entity();

		virtual void init();
		virtual void destroy();

		virtual void tick(float dt);
		virtual void tick_fixed(float dt);

		virtual void editor_layout();

		EntityHandle get_handle() const;

	protected:
		EntityType type;

		u32 original_map_id;
		u32 original_scene_id;
		u32 current_scene_id;
		
		EntityHandle handle;
	};
}

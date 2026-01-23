#pragma once

#include "container/vector.h"
#include "container/deque.h"

#include "entity.h"

namespace ent
{
	struct EntityWorld;

	typedef void EntitySignalListener(Entity *entity, EntityWorld *world, void *context);

	class EntitySignal {
	public:
		EntitySignal();
		~EntitySignal();

		void connect(EntityHandle receiver, EntitySignalListener *listener);
		void emit(EntityWorld *world, void *context);
		void clear();

	private:
		Vector<EntityHandle> receivers;
		Vector<EntitySignalListener *> listeners;
	};

	class EntitySignalDispatcher {
	public:
		EntitySignalDispatcher();
		~EntitySignalDispatcher();

		void enqueue(const EntitySignal &signal, void *context);
		void dispatch(EntityWorld *world);

	private:
		struct Event {
			EntitySignal *signal;
			void *context;
		};

		Deque<Event> events;
	};
}

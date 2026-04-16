#ifndef ENTITY_EVENT_H
#define ENTITY_EVENT_H

typedef enum ENT_EventType
{
	ENT_EventType_None = 0,
	ENT_EventType_SomeRandomThing,
	ENT_EventType_COUNT
}
ENT_EventType;

typedef struct ENT_Event ENT_Event;
struct ENT_Event
{
	ENT_EventType type;
	
	ENT_UID source; // null = global event
	ENT_UID target; // null = broadcast

	f32 timestamp;

	union
	{
		struct
		{
			f32 random_data;
		}
		some_random_thing;
	};
};

#endif // ENTITY_EVENT_H

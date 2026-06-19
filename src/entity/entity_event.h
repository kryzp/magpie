#ifndef ENTITY_EVENT_H
#define ENTITY_EVENT_H

typedef enum E_EventType
{
	E_EventType_None = 0,
	E_EventType_SomeRandomThing,
	E_EventType_COUNT
}
E_EventType;

typedef struct E_Event E_Event;
struct E_Event
{
	E_EventType type;
	
	E_UID source;
	E_UID target; // null = broadcast

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

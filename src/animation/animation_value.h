#ifndef AN_VALUE_H
#define AN_VALUE_H

typedef struct AN_Value AN_Value;
struct AN_Value
{
	u64 key;
	f32 curr;
	f32 target;
};

typedef struct AN_ValueRegistry AN_ValueRegistry;
struct AN_ValueRegistry
{
	AN_Value values[512];
	u32 value_count;
};

//internal void AN_ValueRegistryTick(AN_ValueRegistry *r, f32 dt);

#endif // AN_VALUE_H

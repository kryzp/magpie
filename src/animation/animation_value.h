#ifndef ANIM_VALUE_H
#define ANIM_VALUE_H

typedef struct ANIM_Value ANIM_Value;
struct ANIM_Value
{
    u64 key;
    f32 curr;
    f32 target;
};

typedef struct ANIM_ValueRegistry ANIM_ValueRegistry;
struct ANIM_ValueRegistry
{
    ANIM_Value values[512];
    u32 value_count;
};

//internal void ANIM_ValueRegistryTick(ANIM_ValueRegistry *r, f32 dt);

#endif // ANIM_VALUE_H

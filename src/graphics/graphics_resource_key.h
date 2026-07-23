#ifndef GRAPHICS_RESOURCE_KEY_H
#define GRAPHICS_RESOURCE_KEY_H

typedef struct G_ResourceKey G_ResourceKey;
struct G_ResourceKey
{
	u64 value;
};

internal G_ResourceKey G_ResourceKeyNull(void);
internal b32 G_ResourceKeyIsNull(G_ResourceKey key);
internal b32 G_ResourceKeyMatch(G_ResourceKey a, G_ResourceKey b);

#endif // GRAPHICS_RESOURCE_KEY_H

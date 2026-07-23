#ifndef GRAPHICS_HDI_H
#define GRAPHICS_HDI_H

// once we abstract away the graphics backend we can do
// something like this i rekon?
// todo: also do for audio backend

typedef struct G_ResourceKey G_ResourceKey;
struct G_ResourceKey
{
	u64 value;
};

typedef struct G_HDI G_HDI;
struct G_HDI
{
	void *(*InitAndSelect)(Arena *arena, LOG_Channel log_channel);
	void (*Destroy)(void);
	void (*Select)(void *ctx);

	// ...
};

static G_HDI *g_hdi = NULL;

G_ResourceKey texture = g_hdi->TextureAlloc2D(...);

#endif // GRAPHICS_HDI_H


#define GFX_DEVICE_MANAGED_RESOURCE(mgp_name, resource_name)			\
	typedef struct GFX_##mgp_name##Key GFX_##mgp_name##Key;				\
	struct GFX_##mgp_name##Key											\
	{																	\
		u32 value;														\
	};																	\
	internal GFX_##mgp_name##Key GFX_##mgp_name##KeyNull(void);			\
	internal b32 GFX_##mgp_name##KeyMatch(GFX_##mgp_name##Key a, GFX_##mgp_name##Key b);

#include "graphics_device_managed_resources.inc"

#undef GFX_DEVICE_MANAGED_RESOURCE

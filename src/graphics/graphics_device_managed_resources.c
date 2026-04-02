
#define GFX_DEVICE_MANAGED_RESOURCE(mgp_name, resource_name)			\
	internal GFX_##mgp_name##Key										\
	GFX_##mgp_name##KeyNull(void)										\
	{																	\
		GFX_##mgp_name##Key null_key = {0};								\
		return null_key;												\
	}																	\
	internal b32														\
	GFX_##mgp_name##KeyMatch(GFX_##mgp_name##Key a, GFX_##mgp_name##Key b) \
	{																	\
		return a.value == b.value;										\
	}

#include "graphics_device_managed_resources.inc"

#undef GFX_DEVICE_MANAGED_RESOURCE

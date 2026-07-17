
#define G_DEVICE_MANAGED_RESOURCE(mgp_name, resource_name)			\
	static G_##mgp_name##Key G_##mgp_name##KeyNull(void)										\
	{																	\
		G_##mgp_name##Key null_key = {0};								\
		return null_key;												\
	}																	\
	static b32 G_##mgp_name##KeyIsNull(G_##mgp_name##Key key)					\
	{																	\
		return key.value == 0;											\
	}																	\
	static b32 G_##mgp_name##KeyMatch(G_##mgp_name##Key a, G_##mgp_name##Key b) \
	{																	\
		return a.value == b.value;										\
	}

#include "graphics_device_managed_resources.inc"

#undef G_DEVICE_MANAGED_RESOURCE


/*
 * TODO: Is this really necessary? We could just have
 *       a generic G_Handle that works for everything.
 */

#define G_DEVICE_MANAGED_RESOURCE(mgp_name, resource_name)			\
	typedef struct G_##mgp_name##Key G_##mgp_name##Key;				\
	struct G_##mgp_name##Key											\
	{																	\
		u64 value;														\
	};																	\
	internal G_##mgp_name##Key G_##mgp_name##KeyNull(void);			\
	internal b32 G_##mgp_name##KeyIsNull(G_##mgp_name##Key key);	\
	internal b32 G_##mgp_name##KeyMatch(G_##mgp_name##Key a, G_##mgp_name##Key b);

#include "graphics_device_managed_resources.inc"

#undef G_DEVICE_MANAGED_RESOURCE

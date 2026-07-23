#ifndef GRAPHICS_CAPABILITIES_H
#define GRAPHICS_CAPABILITIES_H

typedef enum G_FeatureTier
{
	G_FeatureTier_Unused,
	G_FeatureTier_Optional,
	G_FeatureTier_Required,
	G_FeatureTier_COUNT
}
G_FeatureTier;

typedef enum G_CapabilityType
{
	G_CapabilityType_SamplerAnisotropy,
	G_CapabilityType_SampleRateShading,
	G_CapabilityType_ShaderI64,
	G_CapabilityType_RayTracingPipeline,
	G_CapabilityType_AccelerationStructure,
	G_CapabilityType_RayQuery,
	G_CapabilityType_COUNT
}
G_CapabilityType;

typedef struct G_Feature G_Feature;
struct G_Feature
{
	G_FeatureTier tier;
	const char *name;

	u32 capability_count;
	G_CapabilityType capabilities[8];
};

typedef struct G_Requirements G_Requirements;
struct G_Requirements
{
	b32 base_extension_count;
	const char **base_extensions;
	
	u32 feature_count;
	G_Feature *features;
};

typedef struct G_Capabilities G_Capabilities;
struct G_Capabilities
{
	b32 set[G_CapabilityType_COUNT];
};

typedef struct G_ResolvedCapabilities G_ResolvedCapabilities;
struct G_ResolvedCapabilities
{
	G_Capabilities detected;
	G_Capabilities enabled;

	b32 meets_requirements;

	const char **extension_names;
	u32 extension_count;
};

internal b32 G_CapabilitiesExtensionListContains(const char **list,
												 u32 count,
												 const char *name);

internal b32 G_CapabilitiesExtensionAvailable(VkExtensionProperties *available,
											  u32 available_count,
											  const char *name);

internal G_Capabilities G_CapabilitiesQuery(VkPhysicalDevice physical_device,
											VkPhysicalDeviceFeatures2 *out_features2,
											LOG_Channel log_channel);

internal G_ResolvedCapabilities G_CapabilitiesResolve(Arena *arena,
													  VkPhysicalDevice physical_device,
													  G_Capabilities detected,
													  const G_Requirements *requirements,
													  LOG_Channel log_channel);

internal u32 G_CapabilitiesScore(G_Capabilities enabled, const G_Requirements *requirements);

#endif // GRAPHICS_CAPABILITIES_H

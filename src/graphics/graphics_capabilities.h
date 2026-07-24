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

typedef enum G_FeatureType
{
	G_FeatureType_RayTracing,
	G_FeatureType_COUNT
}
G_FeatureType;

typedef enum G_CapabilityType
{
	G_CapabilityType_RayTracingPipeline,
	G_CapabilityType_AccelerationStructure,
	G_CapabilityType_RayQuery,
	G_CapabilityType_COUNT
}
G_CapabilityType;

typedef struct G_FeatureDef G_FeatureDef;
struct G_FeatureDef
{
	const char *name;
	u32 capability_count;
	G_CapabilityType capabilities[8];
};

typedef struct G_Feature G_Feature;
struct G_Feature
{
	G_FeatureType type;
	G_FeatureTier tier;
	G_FeatureDef def;
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

typedef struct G_Features G_Features;
struct G_Features
{
	b32 set[G_FeatureType_COUNT];
};

typedef struct G_ResolvedFeatures G_ResolvedFeatures;
struct G_ResolvedFeatures
{
	G_Features enabled;
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

internal G_ResolvedFeatures G_FeaturesResolve(Arena *arena,
											  VkPhysicalDevice physical_device,
											  G_Capabilities detected,
											  const G_Requirements *requirements,
											  LOG_Channel log_channel);

internal u32 G_FeaturesScore(G_Features enabled, const G_Requirements *requirements);

#endif // GRAPHICS_CAPABILITIES_H

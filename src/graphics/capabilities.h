#ifndef GRAPHICS_CAPABILITIES_H
#define GRAPHICS_CAPABILITIES_H

#include "core/types.h"
#include "core/log.h"

#include "container/vector.h"

#include <volk/volk.h>

namespace gfx {

enum FeatureTier {
	FeatureTier_Unused,
	FeatureTier_Optional,
	FeatureTier_Required,
	FeatureTier_MaxEnum
};

enum FeatureType {
	FeatureType_RayTracing,
	FeatureType_MaxEnum
};

enum CapabilityType {
	CapabilityType_RayTracingPipeline,
	CapabilityType_AccelerationStructure,
	CapabilityType_RayQuery,
	CapabilityType_MaxEnum
};

struct FeatureDef {
	const char *name;
	unsigned capability_count;
	CapabilityType capabilities[8];
};

struct Feature {
	FeatureType type;
	FeatureTier tier;
	FeatureDef def;
};

struct FeatureRequest {
	FeatureType type;
	FeatureTier tier;
};

struct Features {
	bool set[FeatureType_MaxEnum] = {};
};

struct Requirements {
	Vector<const char *> base_extensions;
	Vector<Feature> features;

	unsigned Score(const Features &enabled) const;
};

struct Capabilities {
	bool set[CapabilityType_MaxEnum] = {};

	static Capabilities Query(
		VkPhysicalDevice physical_device,
		VkPhysicalDeviceFeatures2 *out_features2,
		LogChannel log_channel
	);
};

struct ResolvedFeatures {
	Features enabled;
	bool meets_requirements = false;
	Vector<const char *> extension_names;

	static ResolvedFeatures Resolve(
		VkPhysicalDevice physical_device,
		const Capabilities &detected,
		const Requirements &requirements,
		LogChannel log_channel
	);
};

const FeatureDef &GetFeatureDef(FeatureType type);

}

#endif // GRAPHICS_CAPABILITIES_H

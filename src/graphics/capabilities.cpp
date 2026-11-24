#include "graphics/capabilities.h"

#include <cstring>

struct CapabilityInfo {
	const char *name;
	unsigned extension_count;
	const char *extensions[4];
};

static const CapabilityInfo capability_infos[] = {
	// CapabilityType_RayTracingPipeline
	{
		"ray_tracing_pipeline",
		3,
		{
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		},
	},
	// CapabilityType_AccelerationStructure
	{
		"acceleration_structure",
		2,
		{
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		},
	},
	// CapabilityType_RayQuery
	{
		"ray_query",
		3,
		{
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		},
	}
};

static_assert(ArraySize(capability_infos) == gfx::CapabilityType_MaxEnum, "Missing capability info.");

static const gfx::FeatureDef feature_defs[] = {
	// FeatureType_RayTracing
	{
		"raytracing",
		3,
		{
			gfx::CapabilityType_RayTracingPipeline,
			gfx::CapabilityType_AccelerationStructure,
			gfx::CapabilityType_RayQuery
		}
	}
};

static_assert(ArraySize(feature_defs) == gfx::FeatureType_MaxEnum, "Missing feature definition.");

const gfx::FeatureDef &gfx::GetFeatureDef(FeatureType type)
{
	return feature_defs[type];
}

static bool ExtensionListContains(const Vector<const char *> &list, const char *name)
{
	for (const char *entry : list) {
		if (strcmp(entry, name) == 0)
			return true;
	}

	return false;
}

static bool ExtensionAvailable(const Vector<VkExtensionProperties> &available, const char *name)
{
	for (const VkExtensionProperties &properties : available) {
		if (strcmp(properties.extensionName, name) == 0)
			return true;
	}

	return false;
}

gfx::Capabilities gfx::Capabilities::Query(
	VkPhysicalDevice physical_device,
	VkPhysicalDeviceFeatures2 *out_features2,
	LogChannel log_channel
)
{
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_query = {};
	rt_pipeline_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_query = {};
	accel_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accel_query.pNext = &rt_pipeline_query;

	VkPhysicalDeviceRayQueryFeaturesKHR ray_query_query = {};
	ray_query_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
	ray_query_query.pNext = &accel_query;

	out_features2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	out_features2->pNext = &ray_query_query;

	vkGetPhysicalDeviceFeatures2(physical_device, out_features2);

	// the chain lives on this stack frame, don't leak it out.
	out_features2->pNext = nullptr;

	Capabilities result;
	result.set[CapabilityType_RayTracingPipeline] = rt_pipeline_query.rayTracingPipeline;
	result.set[CapabilityType_AccelerationStructure] = accel_query.accelerationStructure;
	result.set[CapabilityType_RayQuery] = ray_query_query.rayQuery;

	DebugLogD(log_channel, "Detected Capabilities:");

	for (int i = 0; i < ArraySize(result.set); i++)
		DebugLogD(log_channel, " %s=%d", capability_infos[i].name, result.set[i]);

	return result;
}

gfx::ResolvedFeatures gfx::ResolvedFeatures::Resolve(
	VkPhysicalDevice physical_device,
	const Capabilities &detected,
	const Requirements &requirements,
	LogChannel log_channel
)
{
	ResolvedFeatures result;
	result.meets_requirements = true;

	unsigned available_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &available_count, nullptr);

	Vector<VkExtensionProperties> available(available_count);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &available_count, available.data());

	Vector<const char *> &extensions = result.extension_names;

	for (const char *name : requirements.base_extensions) {
		if (ExtensionAvailable(available, name)) {
			extensions.push_back(name);
		} else {
			result.meets_requirements = false;
			DebugLogE(log_channel, "Missing mandatory base extension: %s", name);
		}
	}

	for (const Feature &f : requirements.features) {
		if (f.tier == FeatureTier_Unused)
			continue;

		bool feature_supported = true;

		for (unsigned j = 0; j < f.def.capability_count && feature_supported; j++) {
			CapabilityType cap = f.def.capabilities[j];
			const CapabilityInfo *info = &capability_infos[cap];

			if (!detected.set[cap]) {
				feature_supported = false;
				break;
			}

			for (unsigned k = 0; k < info->extension_count; k++) {
				if (!ExtensionAvailable(available, info->extensions[k])) {
					feature_supported = false;
					break;
				}
			}
		}

		if (feature_supported) {
			result.enabled.set[f.type] = true;

			for (unsigned j = 0; j < f.def.capability_count; j++) {
				CapabilityType cap = f.def.capabilities[j];
				const CapabilityInfo *info = &capability_infos[cap];

				for (unsigned k = 0; k < info->extension_count; k++) {
					const char *name = info->extensions[k];

					if (!ExtensionListContains(extensions, name))
						extensions.push_back(name);
				}
			}

			DebugLogD(log_channel, "Feature \"%s\" is supported by this device!", f.def.name);
		} else {
			if (f.tier == FeatureTier_Required) {
				result.meets_requirements = false;
				DebugLogE(log_channel, "Feature \"%s\" is required but unsupported by this device.", f.def.name);
			} else {
				DebugLogW(log_channel, "Feature \"%s\" is optional and unsupported by this device.", f.def.name);
			}
		}
	}

	return result;
}

unsigned gfx::Requirements::Score(const Features &enabled) const
{
	unsigned score = 0;

	for (const Feature &feature : features) {
		if (feature.tier == FeatureTier_Unused)
			continue;

		if (enabled.set[feature.type])
			score += 1;
	}

	return score;
}


typedef struct G_CapabilityInfo G_CapabilityInfo;
struct G_CapabilityInfo
{
	const char *name;

	// genuinely arbitrary idk.
	// this is just here from when i followed
	// vulkan-tutorial years ago and i feel
	// sentimental towards this so it just
	// keeps hitching along the ride of refactors.
	u64 score;
	
	u32 extension_count;
	const char *extensions[4];
};

const G_CapabilityInfo g_caps_infos[] = {
	[G_CapabilityType_RayTracingPipeline] = {
		.name = "ray_tracing_pipeline",
		.score = 2,
		.extension_count = 3,
		.extensions = {
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		},
	},
	[G_CapabilityType_AccelerationStructure] = {
		.name = "acceleration_structure",
		.score = 0,
		.extension_count = 2,
		.extensions = {
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		},
	},
	[G_CapabilityType_RayQuery] = {
		.name = "ray_query",
		.score = 0,
		.extension_count = 3,
		.extensions = {
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		},
	}
};

internal b32 G_CapabilitiesExtensionListContains(const char **list,
												 u32 count,
												 const char *name)
{
	for (u32 i = 0; i < count; i++)
	{
		if (CStrCompare(list[i], name) == 0)
			return true;
	}
	
	return false;
}


internal b32 G_CapabilitiesExtensionAvailable(VkExtensionProperties *available,
											  u32 available_count,
											  const char *name)
{
	for (u32 i = 0; i < available_count; i++)
	{
		if (CStrCompare(available[i].extensionName, name) == 0)
			return true;
	}

	return false;
}

internal G_Capabilities G_CapabilitiesQuery(VkPhysicalDevice physical_device,
											VkPhysicalDeviceFeatures2 *out_features2,
											LOG_Channel log_channel)
{
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_query = {0};
	rt_pipeline_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_query = {0};
	accel_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accel_query.pNext = &rt_pipeline_query;

	VkPhysicalDeviceRayQueryFeaturesKHR ray_query_query = {0};
	ray_query_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
	ray_query_query.pNext = &accel_query;

	out_features2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	out_features2->pNext = &ray_query_query;

	vkGetPhysicalDeviceFeatures2(physical_device, out_features2);

	G_Capabilities result = {0};
	result.set[G_CapabilityType_RayTracingPipeline]    = rt_pipeline_query.rayTracingPipeline;
	result.set[G_CapabilityType_AccelerationStructure] = accel_query.accelerationStructure;
	result.set[G_CapabilityType_RayQuery]              = ray_query_query.rayQuery;
	
	DebugLogD(log_channel, "Detected Capabilities:");
	
	for (u32 i = 0; i < ArraySize(result.set); i++)
		DebugLogD(log_channel, " %s=%d", g_caps_infos[i].name, result.set[i]);
	
	return result;
}

internal G_ResolvedCapabilities G_CapabilitiesResolve(Arena *arena,
													  VkPhysicalDevice physical_device,
													  G_Capabilities detected,
													  const G_Requirements *requirements,
													  LOG_Channel log_channel)
{
	G_ResolvedCapabilities result = {0};
	result.detected = detected;
	result.meets_requirements = true;
	
	u32 available_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_count, NULL);
	
	ScratchArena scratch = ScratchBegin(&arena, 1);
	VkExtensionProperties *available = ArenaPushArray(scratch.arena, VkExtensionProperties, available_count);
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &available_count, available);
	
	u32 max_extensions = requirements->base_extension_count;
 
	for (u32 i = 0; i < requirements->feature_count; i++)
	{
		G_Feature *f = &requirements->features[i];
 
		for (u32 j = 0; j < f->capability_count; j++)
			max_extensions += g_caps_infos[f->capabilities[j]].extension_count;
	}
	
	const char **extensions = ArenaPushArray(arena, const char *, max_extensions);
	u32 extension_count = 0;
	
	for (u32 i = 0; i < requirements->base_extension_count; i++)
	{
		const char *name = requirements->base_extensions[i];
		
		if (G_CapabilitiesExtensionAvailable(available, available_count, name))
		{
			extensions[extension_count++] = name;
		}
		else
		{
			result.meets_requirements = false;
			DebugLogE(log_channel, "Missing mandatory base extension: %s", name);
		}
	}
	
	for (u32 i = 0; i < requirements->feature_count; i++)
	{
		G_Feature *f = &requirements->features[i];
		
		if (f->tier == G_FeatureTier_Unused)
			continue;
		
		b32 feature_supported = true;
		
		for (u32 j = 0; j < f->capability_count && feature_supported; j++)
		{
			G_CapabilityType cap = f->capabilities[j];
			const G_CapabilityInfo *info = &g_caps_infos[cap];
			
			if (!detected.set[cap])
			{
				feature_supported = false;
				break;
			}
			
			for (u32 k = 0; k < info->extension_count; k++)
			{
				if (!G_CapabilitiesExtensionAvailable(available, available_count, info->extensions[k]))
				{
					feature_supported = false;
					break;
				}
			}
		}
		
		if (feature_supported)
		{
			for (u32 j = 0; j < f->capability_count; j++)
			{
				G_CapabilityType cap = f->capabilities[j];
				const G_CapabilityInfo *info = &g_caps_infos[cap];
				
				result.enabled.set[cap] = true;
				
				for (u32 k = 0; k < info->extension_count; k++)
				{
					const char *name = info->extensions[k];
					
					if (!G_CapabilitiesExtensionListContains(extensions, extension_count, name))
						extensions[extension_count++] = name;
				}
			}
			
			DebugLogD(log_channel, "Feature \"%s\" is supported by this device!", f->name);
		}
		else
		{
			if (f->tier == G_FeatureTier_Required)
			{
				result.meets_requirements = false;
				DebugLogE(log_channel, "Feature \"%s\" is required but unsupported by this device.", f->name);
			}
			else
			{
				DebugLogW(log_channel, "Feature \"%s\" is optional and unsupported by this device.", f->name);
			}
		}
	}
	
	result.extension_names = extensions;
	result.extension_count = extension_count;
	
	ScratchRelease(&scratch);
	return result;
}

internal u32 G_CapabilitiesScore(G_Capabilities enabled, const G_Requirements *requirements)
{
	u32 score = 0;
 
	for (u32 i = 0; i < requirements->feature_count; i++)
	{
		G_Feature *feature = &requirements->features[i];
 
		if (feature->tier == G_FeatureTier_Unused)
			continue;
 
		for (u32 j = 0; j < feature->capability_count; j++)
		{
			G_CapabilityType cap = feature->capabilities[j];
 
			if (enabled.set[cap])
				score += g_caps_infos[cap].score;
		}
	}
 
	return score;
}

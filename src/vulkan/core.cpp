#include "core.h"

#include "third_party/imgui/imgui_impl_vulkan.h"

#include "core/app.h"
#include "core/common.h"
#include "core/platform.h"

#include "toolbox.h"
#include "validation.h"
#include "swapchain.h"

using namespace mgp;

static std::vector<const char *> getInstanceExtensions(const Platform *platform)
{
	uint32_t extCount = 0;
	const char *const *names = platform->vkGetInstanceExtensions(&extCount);

	if (!names) {
		MGP_ERROR("Unable to get instance extension count.");
	}

	std::vector<const char *> extensions(extCount);

	for (int i = 0; i < extCount; i++) {
		extensions[i] = names[i];
	}

#if MGP_DEBUG
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#if MGP_MAC_SUPPORT
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	extensions.push_back("VK_EXT_metal_surface");
#endif

	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	return extensions;
}

VulkanCore::VulkanCore(const Config &config, const Platform *platform)
	: m_instance()
	, m_device()
	, m_physicalDevice()
	, m_physicalDeviceProperties()
	, m_physicalDeviceFeatures()
	, m_depthFormat()
	, m_maxMsaaSamples()
	, m_bindlessResources()
	, m_vmaAllocator()
	, m_currentFrameIndex(0)
	, m_pipelineProcessCache()
	, m_pipelineCache()
	, m_descriptorLayoutCache()
	, m_imGuiDescriptorPool()
	, m_imGuiImageFormat()
	, m_surface()
//	, m_presentQueue()
	, m_graphicsQueue()
//	, m_computeQueues()
//	, m_transferQueues()
	, m_renderGraph(this)
	, m_platform(platform)
#if MGP_DEBUG
	, m_debugMessenger()
#endif
{
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = config.windowName,
		.applicationVersion = VK_MAKE_API_VERSION(
			config.appVersion.variant,
			config.appVersion.major,
			config.appVersion.minor,
			config.appVersion.patch
		),
		.pEngineName = config.engineName,
		.engineVersion = VK_MAKE_API_VERSION(
			config.engineVersion.variant,
			config.engineVersion.major,
			config.engineVersion.minor,
			config.engineVersion.patch
		),
		.apiVersion = VK_API_VERSION_1_3
	};

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	volkInitialize();

#if MGP_DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
	debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugInfo.pfnUserCallback = vk_validation::vkDebugCallback;
	debugInfo.pUserData = nullptr;

	vk_validation::trySetValidationLayers(createInfo, &debugInfo);
#endif

	// get our extensions
	auto extensions = getInstanceExtensions(m_platform);
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

#if MGP_MAC_SUPPORT
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	// create the vulkan instance
	MGP_VK_CHECK(
		vkCreateInstance(&createInfo, nullptr, &m_instance),
		"Failed to create instance"
	);

	volkLoadInstance(m_instance);

#if MGP_DEBUG
	if (vk_validation::hasValidationLayers())
	{
		MGP_VK_CHECK(
			vk_validation::createDebugUtilsMessengerExt(m_instance, &debugInfo, nullptr, &m_debugMessenger),
			"Failed to create debug messenger"
		);
	}
#endif

	m_pipelineCache.init(this);
	m_descriptorLayoutCache.init(this);

	m_surface.create(this, m_platform);

	enumeratePhysicalDevices(m_surface.getHandle());

	m_depthFormat = vk_toolbox::findDepthFormat(m_physicalDevice);

	findQueueFamilies(m_physicalDevice, m_surface.getHandle());
	createLogicalDevice();
	createPipelineProcessCache();
	createQueues();

	m_bindlessResources.init(this);

	MGP_LOG("Vulkan Core Initialized!");
}

VulkanCore::~VulkanCore()
{
	m_pipelineCache.dispose();
	m_descriptorLayoutCache.dispose();
	m_bindlessResources.destroy();

	m_imGuiDescriptorPool.cleanUp();

	vkDestroyPipelineCache(m_device, m_pipelineProcessCache, nullptr);

	destroyQueues();

	m_surface.destroy();

	vmaDestroyAllocator(m_vmaAllocator);

	vkDestroyDevice(m_device, nullptr);

	MGP_LOG("Vulkan Core Destroyed!");
}

Swapchain *VulkanCore::createSwapchain()
{
	Swapchain *s = new Swapchain(this, m_platform);
	m_imGuiImageFormat = s->getSwapchainImageFormat();
	return s;
}

void VulkanCore::enumeratePhysicalDevices(VkSurfaceKHR surface)
{
	// get the total devices we have
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	// no viable physical device found, exit program
	if (!deviceCount) {
		MGP_ERROR("Failed to find GPUs with Vulkan support!");
	}

	VkPhysicalDeviceProperties2 properties = {};
	properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

	VkPhysicalDeviceFeatures2 features = {};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	vkGetPhysicalDeviceProperties2KHR(devices[0], &properties);
	vkGetPhysicalDeviceFeatures2KHR(devices[0], &features);

	m_physicalDevice = devices[0];
	m_physicalDeviceProperties = properties;
	m_physicalDeviceFeatures = features;

	// try to rank our physical devices and select one accordingly
	bool hasEssentials = false;
	uint32_t usability0 = vk_toolbox::assignPhysicalDeviceUsability(surface, m_physicalDevice, properties, features, &hasEssentials);

	// select the device of the highest usability
	int i = 1;
	for (; i < deviceCount; i++)
	{
		vkGetPhysicalDeviceProperties2KHR(devices[i], &m_physicalDeviceProperties);
		vkGetPhysicalDeviceFeatures2KHR(devices[i], &m_physicalDeviceFeatures);

		uint32_t usability1 = vk_toolbox::assignPhysicalDeviceUsability(surface, devices[i], properties, features, &hasEssentials);

		if (usability1 > usability0 && hasEssentials)
		{
			usability0 = usability1;

			m_physicalDevice = devices[i];
			m_physicalDeviceProperties = properties;
			m_physicalDeviceFeatures = features;
		}
	}

	// get the actual number of samples we can take
	m_maxMsaaSamples = vk_toolbox::getMaxUsableSampleCount(m_physicalDeviceProperties);

	if (m_physicalDevice == VK_NULL_HANDLE)
		MGP_ERROR("Unable to find a suitable GPU!");

	MGP_LOG("Selected a suitable GPU: %d", i);
}

void VulkanCore::createLogicalDevice()
{
	const std::vector<float> QUEUE_PRIORITIES = { 1.0f };

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	populateQueueCreateInfos(queueCreateInfos, QUEUE_PRIORITIES);

	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeaturesExt = {};
	descriptorIndexingFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	descriptorIndexingFeaturesExt.runtimeDescriptorArray = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingPartiallyBound = VK_TRUE;
	descriptorIndexingFeaturesExt.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeaturesExt.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeaturesExt.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeaturesExt.pNext = nullptr;

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeaturesExt = {};
	dynamicRenderingFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRenderingFeaturesExt.dynamicRendering = VK_TRUE;
	dynamicRenderingFeaturesExt.pNext = &descriptorIndexingFeaturesExt;

	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeaturesExt = {};
	bufferDeviceAddressFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferDeviceAddressFeaturesExt.bufferDeviceAddress = VK_TRUE;
	bufferDeviceAddressFeaturesExt.pNext = &dynamicRenderingFeaturesExt;

	VkPhysicalDeviceSynchronization2Features synchronisation2FeaturesExt = {};
	synchronisation2FeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	synchronisation2FeaturesExt.synchronization2 = VK_TRUE;
	synchronisation2FeaturesExt.pNext = &bufferDeviceAddressFeaturesExt;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.enabledExtensionCount = MGP_ARRAY_LENGTH(vk_toolbox::DEVICE_EXTENSIONS);
	createInfo.ppEnabledExtensionNames = vk_toolbox::DEVICE_EXTENSIONS;
	createInfo.pEnabledFeatures = &m_physicalDeviceFeatures.features;
	createInfo.pNext = &synchronisation2FeaturesExt;

#if MGP_DEBUG
	
	// enable the validation layers on the device
	if (vk_validation::hasValidationLayers())
	{
		createInfo.enabledLayerCount = MGP_ARRAY_LENGTH(vk_validation::VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = vk_validation::VALIDATION_LAYERS;
	}

	MGP_LOG("Enabled validation layers!");

#endif

	// create it
	MGP_VK_CHECK(
		vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device),
		"Failed to create logical device"
	);

	volkLoadDevice(m_device);

	// init memory allocator
	createVmaAllocator();

	// print out current device version
	uint32_t version = 0;
	VkResult result = vkEnumerateInstanceVersion(&version);

	if (result == VK_SUCCESS)
	{
		uint32_t major = VK_API_VERSION_MAJOR(version);
		uint32_t minor = VK_API_VERSION_MINOR(version);

		MGP_LOG("Using Vulkan %d.%d", major, minor);
	}
	else
	{
		MGP_LOG("Failed to retrieve Vulkan version.");
	}

	MGP_LOG("Created logical device!");
}

void VulkanCore::createPipelineProcessCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.pNext = nullptr;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = nullptr;

	MGP_VK_CHECK(
		vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipelineProcessCache),
		"Failed to process pipeline cache"
	);

	MGP_LOG("Created graphics pipeline process cache!");
}

void VulkanCore::createVmaAllocator()
{
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.physicalDevice = m_physicalDevice;
	allocatorCreateInfo.device = m_device;
	allocatorCreateInfo.instance = m_instance;
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

	MGP_VK_CHECK(
		vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator),
		"Failed to create memory allocator"
	);

	MGP_LOG("Created memory allocator!");
}

void VulkanCore::createQueues()
{
//	m_presentQueue.create(this, 0);
	m_graphicsQueue.create(this, 0);

//	for (int i = 0; i < m_computeQueues.size(); i++)
//		m_computeQueues[i].create(this, i);

//	for (int i = 0; i < m_transferQueues.size(); i++)
//		m_transferQueues[i].create(this, i);
}

void VulkanCore::destroyQueues()
{
//	m_presentQueue.destroy();
	m_graphicsQueue.destroy();

//	for (auto &q : m_computeQueues)
//		q.destroy();

//	for (auto &q : m_transferQueues)
//		q.destroy();
}

void VulkanCore::populateQueueCreateInfos(std::vector<VkDeviceQueueCreateInfo> &infos, const std::vector<float> &priorities)
{
	/*
	infos.push_back((VkDeviceQueueCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = m_presentQueue.getFamilyIndex(),
			.queueCount = (unsigned)priorities.size(),
			.pQueuePriorities = priorities.data()
	});
	*/

	infos.push_back((VkDeviceQueueCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = m_graphicsQueue.getFamilyIndex(),
		.queueCount = (unsigned)priorities.size(),
		.pQueuePriorities = priorities.data()
	});

	/*
	for (auto &q : m_computeQueues)
	{
		infos.push_back((VkDeviceQueueCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = q.getFamilyIndex(),
			.queueCount = (unsigned)priorities.size(),
			.pQueuePriorities = priorities.data()
		});
	}

	for (auto &q : m_transferQueues)
	{
		infos.push_back((VkDeviceQueueCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = q.getFamilyIndex(),
			.queueCount = (unsigned)priorities.size(),
			.pQueuePriorities = priorities.data()
		});
	}
	*/
}

// todo: move into vk_toolbox
void VulkanCore::findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	if (!queueFamilyCount)
		MGP_ERROR("Failed to find any queue families!");

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	struct QueuesFound
	{
//		int present;
		int graphics;
//		int compute;
//		int transfer;
	};

	QueuesFound numQueuesFound = {};

	for (int i = 0; i < queueFamilyCount; i++)
	{
		if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && numQueuesFound.graphics == 0)
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			if (presentSupport)
			{
				m_graphicsQueue.setFamilyIndex(i);

				numQueuesFound.graphics++;

				continue;
			}
		}

		/*
		if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && numQueuesFound.compute < queueFamilies[i].queueCount)
		{
			m_computeQueues.emplace_back();
			m_computeQueues.back().setFamilyIndex(i);

			numQueuesFound.compute++;

			continue;
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && numQueuesFound.transfer < queueFamilies[i].queueCount)
		{
			m_transferQueues.emplace_back();
			m_transferQueues.back().setFamilyIndex(i);

			numQueuesFound.transfer++;

			continue;
		}
		*/
	}
}

void VulkanCore::deviceWaitIdle() const
{
	vkDeviceWaitIdle(m_device);
}

void VulkanCore::waitForFence(const VkFence &fence) const
{
	vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void VulkanCore::resetFence(const VkFence &fence) const
{
	vkResetFences(m_device, 1, &fence);
}

const VkInstance &VulkanCore::getInstance() const
{
	return m_instance;
}

const VkDevice &VulkanCore::getLogicalDevice() const
{
	return m_device;
}

const VkPhysicalDevice &VulkanCore::getPhysicalDevice() const
{
	return m_physicalDevice;
}

const VkPhysicalDeviceProperties2 &VulkanCore::getPhysicalDeviceProperties() const
{
	return m_physicalDeviceProperties;
}

const VkPhysicalDeviceFeatures2 &VulkanCore::getPhysicalDeviceFeatures() const
{
	return m_physicalDeviceFeatures;
}

const VkSampleCountFlagBits VulkanCore::getMaxMSAASamples() const
{
	return m_maxMsaaSamples;
}

PipelineCache &VulkanCore::getPipelineCache()
{
	return m_pipelineCache;
}

const PipelineCache &VulkanCore::getPipelineCache() const
{
	return m_pipelineCache;
}

DescriptorLayoutCache &VulkanCore::getDescriptorLayoutCache()
{
	return m_descriptorLayoutCache;
}

const DescriptorLayoutCache &VulkanCore::getDescriptorLayoutCache() const
{
	return m_descriptorLayoutCache;
}

const Surface &VulkanCore::getSurface() const
{
	return m_surface;
}

const VmaAllocator &VulkanCore::getVMAAllocator() const
{
	return m_vmaAllocator;
}

/*
Queue &VulkanCore::getPresentQueue()
{
	return m_presentQueue;
}

const Queue &VulkanCore::getPresentQueue() const
{
	return m_presentQueue;
}
*/

Queue &VulkanCore::getGraphicsQueue()
{
	return m_graphicsQueue;
}

const Queue &VulkanCore::getGraphicsQueue() const
{
	return m_graphicsQueue;
}

/*
std::vector<Queue> &VulkanCore::getComputeQueues()
{
	return m_computeQueues;
}

const std::vector<Queue> &VulkanCore::getComputeQueues() const
{
	return m_computeQueues;
}

std::vector<Queue> &VulkanCore::getTransferQueues()
{
	return m_transferQueues;
}

const std::vector<Queue> &VulkanCore::getTransferQueues() const
{
	return m_transferQueues;
}
*/

BindlessResources &VulkanCore::getBindlessResources()
{
	return m_bindlessResources;
}

const BindlessResources &VulkanCore::getBindlessResources() const
{
	return m_bindlessResources;
}

void VulkanCore::nextFrame()
{
	m_currentFrameIndex = (m_currentFrameIndex + 1) % Queue::FRAMES_IN_FLIGHT;

//	vkQueueWaitIdle(m_presentQueue.getHandle());
//	m_presentQueue.getFrame(m_currentFrameIndex).pool.reset();

	vkQueueWaitIdle(m_graphicsQueue.getHandle());
	m_graphicsQueue.getFrame(m_currentFrameIndex).pool.reset();

	/*
	for (auto &q : m_computeQueues)
	{
		vkQueueWaitIdle(q.getHandle());
		q.getFrame(m_currentFrameIndex).pool.reset();
	}

	for (auto &q : m_transferQueues)
	{
		vkQueueWaitIdle(q.getHandle());
		q.getFrame(m_currentFrameIndex).pool.reset();
	}
	*/
}

int VulkanCore::getCurrentFrameIndex() const
{
	return m_currentFrameIndex;
}

RenderGraph &VulkanCore::getRenderGraph()
{
	return m_renderGraph;
}

const RenderGraph &VulkanCore::getRenderGraph() const
{
	return m_renderGraph;
}

VkFormat VulkanCore::getDepthFormat() const
{
	return m_depthFormat;
}

void VulkanCore::initImGui()
{
	m_imGuiDescriptorPool.init(this, 1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					1.0f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	1.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			1.0f }
	});

	ImGui_ImplVulkan_InitInfo info = {};
	info.Instance = m_instance;
	info.PhysicalDevice = m_physicalDevice;
	info.Device = m_device;
	info.QueueFamily = m_graphicsQueue.getFamilyIndex();
	info.Queue = m_graphicsQueue.getHandle();
	info.PipelineCache = m_pipelineProcessCache;
	info.DescriptorPool = m_imGuiDescriptorPool.getPool();
	info.Allocator = nullptr;
	info.MinImageCount = Queue::FRAMES_IN_FLIGHT;
	info.ImageCount = Queue::FRAMES_IN_FLIGHT;
	info.CheckVkResultFn = nullptr;
	info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	info.UseDynamicRendering = true;
	info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_imGuiImageFormat;

	ImGui_ImplVulkan_Init(&info);

	ImGui_ImplVulkan_CreateFontsTexture();
}

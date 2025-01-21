#include "backend.h"
#include "util.h"
#include "texture.h"
#include "backbuffer.h"
#include "../platform.h"
#include "../math/calc.h"
#include "../third_party/volk.h"

llt::VulkanBackend* llt::g_vulkanBackend = nullptr;

using namespace llt;

#if LLT_DEBUG

static bool g_debugEnableValidationLayers = false;

/*
 * Go through all the required layers to see
 * if there is support for a validation layer that
 * we need. If there is then it means
 * we must have support for validation layers.
 */
static bool debugHasValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	VkLayerProperties availableLayers[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	for (int i = 0; i < LLT_ARRAY_LENGTH(vkutil::VALIDATION_LAYERS); i++)
	{
		bool hasLayer = false;
		const char* layerName0 = vkutil::VALIDATION_LAYERS[i];

		for (int j = 0; j < layerCount; j++)
		{
			const char* layerName1 = availableLayers[j].layerName;

			if (cstr::compare(layerName0, layerName1) == 0) {
				hasLayer = true;
				break;
			}
		}

		if (!hasLayer) {
			return false;
		}
	}

	return true;
}

/*
 * Hook for the validation layer.
 * Calls to the engine error function.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		LLT_ERROR("Validation Layer (SEVERITY: %d) (TYPE: %d): %s", messageSeverity, messageType, pCallbackData->pMessage);
	}

	return VK_FALSE;
}

/*
 * Initializes the vulkan debugging messenger.
 */
static VkResult debugCreateDebugUtilsMessengerExt(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
	const VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* debugMessenger
)
{
	auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn) {
		return fn(instance, createInfo, allocator, debugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

/*
 * Destroys the vulkan debugging messenger.
 */
static void debugDestroyDebugUtilsMessengerExt(
	VkInstance instance,
	VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks* allocator
)
{
	auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (fn) {
		fn(instance, messenger, allocator);
	}
}

/*
 * Set all of the settings for how the debugging messenger should output information.
 */
static void debugPopulateDebugUtilsMessengerCreateInfoExt(VkDebugUtilsMessengerCreateInfoEXT* info)
{
	info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	info->pfnUserCallback = vkDebugCallback;
	info->pUserData = nullptr;
}

#endif // LLT_DEBUG

/*
 * Retrieve all the possible extensions for our Vulkan backend.
 */
static Vector<const char*> getInstanceExtensions()
{
	uint32_t extCount = 0;
	const char* const* names = g_platform->vkGetInstanceExtensions(&extCount);

	if (!names) {
		LLT_ERROR("Unable to get instance extension count.");
	}

	Vector<const char*> extensions(extCount);

	for (int i = 0; i < extCount; i++) {
		extensions[i] = names[i];
	}

#if LLT_DEBUG
	extensions.pushBack(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensions.pushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // LLT_DEBUG

#if LLT_MAC_SUPPORT
	extensions.pushBack(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	extensions.pushBack("VK_EXT_metal_surface");
#endif // LLT_MAC_SUPPORT

	extensions.pushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	return extensions;
}

VulkanBackend::VulkanBackend(const Config& config)
	: m_instance()
	, m_device()
	, m_physicalData()
	, m_maxMsaaSamples()
	, m_swapChainImageFormat()
	, m_vmaAllocator()
	, m_pipelineCache()
	, m_pipelineLayoutCache()
	, m_graphicsQueue()
	, m_computeQueues()
	, m_transferQueues()
	, m_pushConstants()
	, m_pipelineProcessCache()
	, m_computeFinishedSemaphores()
	, m_backbuffer()
	, m_currentFrameIdx()
	, m_currentRenderTarget()
	, m_waitOnCompute()
#if LLT_DEBUG
	, m_debugMessenger()
#endif // LLT_DEBUG
{
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = config.name,
		.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.pEngineName = "Lilythorn",
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info
	};

	volkInitialize();

#if LLT_DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

	if (debugHasValidationLayerSupport())
	{
		LLT_LOG("Validation layer support verified.");

		debugPopulateDebugUtilsMessengerCreateInfoExt(&debugCreateInfo);

		createInfo.enabledLayerCount = LLT_ARRAY_LENGTH(vkutil::VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = vkutil::VALIDATION_LAYERS;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

		g_debugEnableValidationLayers = true;
	}
	else
	{
		LLT_LOG("No validation layer support.");

		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;

		g_debugEnableValidationLayers = false;
	}
#endif // LLT_DEBUG

	// get our extensions
	auto extensions = getInstanceExtensions();
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

#if LLT_MAC_SUPPORT
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif // LLT_MAC_SUPPORT

	// create the vulkan instance
	LLT_VK_CHECK(
		vkCreateInstance(&createInfo, nullptr, &m_instance),
		"Failed to create instance"
	);

	volkLoadInstance(m_instance);

#if LLT_DEBUG
	LLT_VK_CHECK(
		debugCreateDebugUtilsMessengerExt(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger),
		"Failed to create debug messenger"
	);
#endif // LLT_DEBUG

	// set up all of the core managers of resources
	g_gpuBufferManager		= new GPUBufferMgr();
	g_shaderBufferManager 	= new ShaderBufferMgr();
	g_textureManager      	= new TextureMgr();
	g_shaderManager       	= new ShaderMgr();
	g_renderTargetManager 	= new RenderTargetMgr();

	// finished :D
	LLT_LOG("Vulkan Backend Initialized!");
}

VulkanBackend::~VulkanBackend()
{
	// wait until we are synced-up with the gpu before proceeding
	syncStall();

	// //

	m_backbuffer->cleanUpSwapChain();

	delete g_renderTargetManager;
	delete g_shaderManager;
	delete g_textureManager;
	delete g_gpuBufferManager;

	m_backbuffer->cleanUpTextures();

	clearPipelineCache();

	delete g_shaderBufferManager;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyFence(m_device, m_graphicsQueue.getFrame(i).inFlightFence, nullptr);
		vkDestroyCommandPool(m_device, m_graphicsQueue.getFrame(i).commandPool, nullptr);

		vkDestroySemaphore(m_device, m_computeFinishedSemaphores[i], nullptr);

		for (int j = 0; j < m_computeQueues.size(); j++) {
			vkDestroyFence(m_device, m_computeQueues[j].getFrame(i).inFlightFence, nullptr);
			vkDestroyCommandPool(m_device, m_computeQueues[j].getFrame(i).commandPool, nullptr);
		}

		for (int j = 0; j < m_transferQueues.size(); j++) {
			vkDestroyFence(m_device, m_transferQueues[j].getFrame(i).inFlightFence, nullptr);
			vkDestroyCommandPool(m_device, m_transferQueues[j].getFrame(i).commandPool, nullptr);
		}
	}

	delete m_backbuffer;

	vmaDestroyAllocator(m_vmaAllocator);

	vkDestroyDevice(m_device, nullptr);

#if LLT_DEBUG
	// if we have validation layers then destroy them
	if (g_debugEnableValidationLayers) {
		debugDestroyDebugUtilsMessengerExt(m_instance, m_debugMessenger, nullptr);
		LLT_LOG("Destroyed validation layers!");
	}
#endif // LLT_DEBUG

	// finished with vulkan, may now safely destroy the vulkan instance
	vkDestroyInstance(m_instance, nullptr);

	// //

	LLT_LOG("Vulkan Backend Destroyed!");
}

void VulkanBackend::enumeratePhysicalDevices()
{
	VkSurfaceKHR surface = m_backbuffer->getSurface();

	// get the total devices we have
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	// no viable physical device found, exit program
	if (!deviceCount) {
		LLT_ERROR("Failed to find GPUs with Vulkan support!");
	}

	VkPhysicalDeviceProperties properties = {};
	VkPhysicalDeviceFeatures features = {};

	Vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	vkGetPhysicalDeviceProperties(devices[0], &properties);
	vkGetPhysicalDeviceFeatures(devices[0], &features);

	m_physicalData.device = devices[0];
	m_physicalData.properties = properties;
	m_physicalData.features = features;

	// get the actual number of samples we can take
	m_maxMsaaSamples = getMaxUsableSampleCount();

	// try to rank our physical devices and select one accordingly
	bool hasEssentials = false;
	uint32_t usability0 = vkutil::assignPhysicalDeviceUsability(surface, m_physicalData.device, properties, features, &hasEssentials);

	// select the device of the highest usability
	int i = 1;
	for (; i < deviceCount; i++)
	{
		vkGetPhysicalDeviceProperties(devices[i], &m_physicalData.properties);
		vkGetPhysicalDeviceFeatures(devices[i], &m_physicalData.features);

		uint32_t usability1 = vkutil::assignPhysicalDeviceUsability(surface, devices[i], properties, features, &hasEssentials);

		if (usability1 > usability0 && hasEssentials)
		{
			usability0 = usability1;

			m_physicalData.device = devices[i];
			m_physicalData.properties = properties;
			m_physicalData.features = features;

			m_maxMsaaSamples = getMaxUsableSampleCount();
		}
	}

	// still no device with the abilities we need, exit the program
	if (m_physicalData.device == VK_NULL_HANDLE) {
		LLT_ERROR("Unable to find a suitable GPU!");
	}

	LLT_LOG("Selected a suitable GPU: %d", i);
}

void VulkanBackend::createLogicalDevice()
{
	const float QUEUE_PRIORITY = 1.0f;

	Vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	queueCreateInfos.pushBack({
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = m_graphicsQueue.getFamilyIdx().value(),
		.queueCount = 1,
		.pQueuePriorities = &QUEUE_PRIORITY
	});

	for (auto& computeQueue : m_computeQueues)
	{
		if (!computeQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		queueCreateInfos.pushBack({
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = computeQueue.getFamilyIdx().value(),
			.queueCount = 1,
			.pQueuePriorities = &QUEUE_PRIORITY
		});
	}

	for (auto& transferQueue : m_transferQueues)
	{
		if (!transferQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		queueCreateInfos.pushBack({
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = transferQueue.getFamilyIdx().value(),
			.queueCount = 1,
			.pQueuePriorities = &QUEUE_PRIORITY
		});
	}

	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeaturesExt = {};
	descriptorIndexingFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

	descriptorIndexingFeaturesExt.descriptorBindingPartiallyBound = VK_TRUE;

	descriptorIndexingFeaturesExt.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeaturesExt.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeaturesExt.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

	descriptorIndexingFeaturesExt.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

	descriptorIndexingFeaturesExt.pNext = nullptr;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesExt = {};
	dynamicRenderingFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamicRenderingFeaturesExt.dynamicRendering = VK_TRUE;
	dynamicRenderingFeaturesExt.pNext = &descriptorIndexingFeaturesExt;

	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeaturesExt = {};
	bufferDeviceAddressFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferDeviceAddressFeaturesExt.bufferDeviceAddress = VK_TRUE;
	bufferDeviceAddressFeaturesExt.pNext = &dynamicRenderingFeaturesExt;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.ppEnabledExtensionNames = vkutil::DEVICE_EXTENSIONS;
	createInfo.enabledExtensionCount = LLT_ARRAY_LENGTH(vkutil::DEVICE_EXTENSIONS);
	createInfo.pEnabledFeatures = &m_physicalData.features;
	createInfo.pNext = &bufferDeviceAddressFeaturesExt;

#if LLT_DEBUG
	// enable the validation layers on the device
	if (g_debugEnableValidationLayers) {
		createInfo.enabledLayerCount = LLT_ARRAY_LENGTH(vkutil::VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = vkutil::VALIDATION_LAYERS;
	}
	LLT_LOG("Enabled validation layers!");
#endif // LLT_DEBUG

	// create it
	LLT_VK_CHECK(
		vkCreateDevice(m_physicalData.device, &createInfo, nullptr, &m_device),
		"Failed to create logical device"
	);

	volkLoadDevice(m_device);

	// get the device queues for the four core vulkan families
	VkQueue tmpQueue;

	vkGetDeviceQueue(m_device, m_graphicsQueue.getFamilyIdx().value(), 0, &tmpQueue);
	m_graphicsQueue.init(tmpQueue);

	for (auto& computeQueue : m_computeQueues)
	{
		if (!computeQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		vkGetDeviceQueue(m_device, computeQueue.getFamilyIdx().value(), 0, &tmpQueue);
		computeQueue.init(tmpQueue);
	}

	for (auto& transferQueue : m_transferQueues)
	{
		if (!transferQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		vkGetDeviceQueue(m_device, transferQueue.getFamilyIdx().value(), 0, &tmpQueue);
		transferQueue.init(tmpQueue);
	}

	// init memory allocator
	createVmaAllocator();

	// print out current device version
	uint32_t version = 0;
	VkResult result = vkEnumerateInstanceVersion(&version);

	if (result == VK_SUCCESS)
	{
		uint32_t major = VK_VERSION_MAJOR(version);
		uint32_t minor = VK_VERSION_MINOR(version);

		LLT_LOG("Using version: %d.%d", major, minor);
	}

	LLT_LOG("Created logical device!");
}

void VulkanBackend::createVmaAllocator()
{
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.physicalDevice = m_physicalData.device;
	allocatorCreateInfo.device = m_device;
	allocatorCreateInfo.instance = m_instance;
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

	LLT_VK_CHECK(
		vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator),
		"Failed to create memory allocator"
	);

	LLT_LOG("Created memory allocator!");
}

void VulkanBackend::createPipelineProcessCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.pNext = nullptr;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = nullptr;

	LLT_VK_CHECK(
		vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipelineProcessCache),
		"Failed to process pipeline cache"
	);

	LLT_LOG("Created graphics pipeline process cache!");
}

void VulkanBackend::createComputeResources()
{
	VkSemaphoreCreateInfo computeSemaphoreCreateInfo = {};
	computeSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		LLT_VK_CHECK(
			vkCreateSemaphore(m_device, &computeSemaphoreCreateInfo, nullptr, &m_computeFinishedSemaphores[i]),
			"Failed to create compute finished semaphore"
		);
	}

	LLT_LOG("Created compute resources!");
}

void VulkanBackend::createCommandPools()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		createInfo.queueFamilyIndex = m_graphicsQueue.getFamilyIdx().value();

		LLT_VK_CHECK(
			vkCreateCommandPool(m_device, &createInfo, nullptr, &m_graphicsQueue.getFrame(i).commandPool),
			"Failed to create graphics command pool"
		);

		for (int j = 0; j < m_computeQueues.size(); j++)
		{
			createInfo.queueFamilyIndex = m_computeQueues[j].getFamilyIdx().value();

			LLT_VK_CHECK(
				vkCreateCommandPool(m_device, &createInfo, nullptr, &m_computeQueues[j].getFrame(i).commandPool),
				"Failed to create compute command pool"
			);
		}

		for (int j = 0; j < m_transferQueues.size(); j++)
		{
			createInfo.queueFamilyIndex = m_transferQueues[j].getFamilyIdx().value();

			LLT_VK_CHECK(
				vkCreateCommandPool(m_device, &createInfo, nullptr, &m_transferQueues[j].getFrame(i).commandPool),
				"Failed to create transfer command pool"
			);
		}
	}

	LLT_LOG("Created command pool!");
}

void VulkanBackend::createCommandBuffers()
{
	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = m_graphicsQueue.getFrame(i).commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		LLT_VK_CHECK(
			vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &m_graphicsQueue.getFrame(i).commandBuffer),
			"Failed to create graphics command buffer"
		);

		for (int j = 0; j < m_computeQueues.size(); j++)
		{
			auto& computeQueue = m_computeQueues[j];

			commandBufferAllocateInfo.commandPool = computeQueue.getFrame(i).commandPool;

			LLT_VK_CHECK(
				vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &computeQueue.getFrame(i).commandBuffer),
				"Failed to create compute command buffer"
			);
		}

		for (int j = 0; j < m_transferQueues.size(); j++)
		{
			auto& transferQueue = m_transferQueues[j];

			commandBufferAllocateInfo.commandPool = transferQueue.getFrame(i).commandPool;

			LLT_VK_CHECK(
				vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &transferQueue.getFrame(i).commandBuffer),
				"Failed to create transfer command buffer"
			);
		}
	}

	LLT_LOG("Created command buffer!");
}

Backbuffer* VulkanBackend::createBackbuffer()
{
	Backbuffer* backbuffer = new Backbuffer();
	backbuffer->createSurface();

	m_backbuffer = backbuffer;

	enumeratePhysicalDevices();

	findQueueFamilies(m_physicalData.device, m_backbuffer->getSurface());

	createLogicalDevice();
	createPipelineProcessCache();

	createCommandPools();
	createCommandBuffers();

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		LLT_VK_CHECK(
			vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_graphicsQueue.getFrame(i).inFlightFence),
			"Failed to create graphics in flight fence"
		);

		for (int j = 0; j < m_computeQueues.size(); j++)
		{
			LLT_VK_CHECK(
				vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_computeQueues[j].getFrame(i).inFlightFence),
				"Failed to create compute in flight fence"
			);
		}

		for (int j = 0; j < m_transferQueues.size(); j++)
		{
			LLT_VK_CHECK(
				vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_transferQueues[j].getFrame(i).inFlightFence),
				"Failed to create transfer in flight fence"
			);
		}
	}

	LLT_LOG("Created in flight fence!");

	createComputeResources();

	backbuffer->create();

	return backbuffer;
}

void VulkanBackend::clearPipelineCache()
{
	for (auto& [id, cache] : m_pipelineCache) {
		vkDestroyPipeline(m_device, cache, nullptr);
	}

	m_pipelineCache.clear();

	for (auto& [id, cache] : m_pipelineLayoutCache) {
		vkDestroyPipelineLayout(m_device, cache, nullptr);
	}

	m_pipelineLayoutCache.clear();

	vkDestroyPipelineCache(m_device, m_pipelineProcessCache, nullptr);
}

VkSampleCountFlagBits VulkanBackend::getMaxUsableSampleCount() const
{
	VkSampleCountFlags counts =
		m_physicalData.properties.limits.framebufferColorSampleCounts &
		m_physicalData.properties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT) {
		return VK_SAMPLE_COUNT_64_BIT;
	} else if (counts & VK_SAMPLE_COUNT_32_BIT) {
		return VK_SAMPLE_COUNT_32_BIT;
	} else if (counts & VK_SAMPLE_COUNT_16_BIT) {
		return VK_SAMPLE_COUNT_16_BIT;
	} else if (counts & VK_SAMPLE_COUNT_8_BIT) {
		return VK_SAMPLE_COUNT_8_BIT;
	} else if (counts & VK_SAMPLE_COUNT_4_BIT) {
		return VK_SAMPLE_COUNT_4_BIT;
	} else if (counts & VK_SAMPLE_COUNT_2_BIT) {
		return VK_SAMPLE_COUNT_2_BIT;
	} else if (counts & VK_SAMPLE_COUNT_1_BIT) {
		return VK_SAMPLE_COUNT_1_BIT;
	}

	LLT_ERROR("Could not find a maximum usable sample count!");
	return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanBackend::findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	if (!queueFamilyCount) {
		LLT_ERROR("Failed to find any queue families!");
	}

	Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t numQueuesFound[QUEUE_FAMILY_MAX_ENUM] = {};

	for (int i = 0; i < queueFamilyCount; i++)
	{
		if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && numQueuesFound[i] == 0)
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			if (presentSupport)
			{
				m_graphicsQueue.setData(QUEUE_FAMILY_GRAPHICS, i);
				numQueuesFound[QUEUE_FAMILY_GRAPHICS]++;
				continue;
			}
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && numQueuesFound[QUEUE_FAMILY_COMPUTE] < queueFamilies[i].queueCount)
		{
			m_computeQueues.emplaceBack();
			m_computeQueues.back().setData(QUEUE_FAMILY_COMPUTE, i);
			numQueuesFound[QUEUE_FAMILY_COMPUTE]++;
			continue;
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && numQueuesFound[QUEUE_FAMILY_TRANSFER] < queueFamilies[i].queueCount)
		{
			m_transferQueues.emplaceBack();
			m_transferQueues.back().setData(QUEUE_FAMILY_TRANSFER, i);
			numQueuesFound[QUEUE_FAMILY_TRANSFER]++;
			continue;
		}
	}
}

void VulkanBackend::onWindowResize(int width, int height)
{
	m_backbuffer->onWindowResize(width, height);
}

void VulkanBackend::setPushConstants(ShaderParameters& params)
{
	m_pushConstants = params.getPackedConstants();
}

void VulkanBackend::resetPushConstants()
{
	m_pushConstants.clear();
}

int VulkanBackend::getCurrentFrameIdx() const
{
	return m_currentFrameIdx;
}

GenericRenderTarget* VulkanBackend::getRenderTarget()
{
	return m_currentRenderTarget ? m_currentRenderTarget : m_backbuffer;
}

const GenericRenderTarget* VulkanBackend::getRenderTarget() const
{
	return m_currentRenderTarget ? m_currentRenderTarget : m_backbuffer;
}

VkCommandBuffer VulkanBackend::getGraphicsCommandBuffer()
{
	return m_graphicsQueue.getCurrentFrame().commandBuffer;
}

// todo
VkCommandBuffer VulkanBackend::getTransferCommandBuffer(int idx)
{
//	if (m_transferQueues.size() > 0)
//		return m_transferQueues[idx].getCurrentFrame().commandBuffer;

	return getGraphicsCommandBuffer();
}

VkCommandBuffer VulkanBackend::getComputeCommandBuffer(int idx)
{
	return m_computeQueues[idx].getCurrentFrame().commandBuffer;
}

VkCommandPool VulkanBackend::getGraphicsCommandPool()
{
	return m_graphicsQueue.getCurrentFrame().commandPool;
}

// todo
VkCommandPool VulkanBackend::getTransferCommandPool(int idx)
{
//	if (m_transferQueues.size() > 0)
//		return m_transferQueues[idx].getCurrentFrame().commandPool;

	return getGraphicsCommandPool();
}

VkCommandPool VulkanBackend::getComputeCommandPool(int idx)
{
	return m_computeQueues[idx].getCurrentFrame().commandPool;
}

void VulkanBackend::beginGraphics(GenericRenderTarget* target)
{
	cauto& currentFrame = m_graphicsQueue.getCurrentFrame();
	cauto& currentBuffer = getGraphicsCommandBuffer();

	m_currentRenderTarget = target;

	vkWaitForFences(g_vulkanBackend->m_device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

	vkResetCommandPool(g_vulkanBackend->m_device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	LLT_VK_CHECK(
		vkBeginCommandBuffer(currentBuffer, &commandBufferBeginInfo),
		"Failed to begin recording command buffer"
	);

	getRenderTarget()->beginGraphics(currentBuffer);

	VkRenderingInfoKHR renderInfo = getRenderTarget()->getRenderInfo()->getInfo();

	vkCmdBeginRendering(currentBuffer, &renderInfo);
}

void VulkanBackend::endGraphics()
{
	cauto& currentTarget = getRenderTarget();
	cauto& currentFrame = m_graphicsQueue.getCurrentFrame();
	cauto& currentBuffer = getGraphicsCommandBuffer();

	vkCmdEndRendering(currentBuffer);

	currentTarget->endGraphics(currentBuffer);

	LLT_VK_CHECK(
		vkEndCommandBuffer(currentBuffer),
		"Failed to record command buffer"
	);

	int signalSemaphoreCount = currentTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? 1 : 0;
	const VkSemaphore* queueFinishedSemaphore = currentTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? &((Backbuffer*)currentTarget)->getRenderFinishedSemaphore() : VK_NULL_HANDLE;

	int waitSemaphoreCount = currentTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? 1 : 0;

	VkSemaphore waitForFinishSemaphores[2] = { // todo: this is pretty ugly and probably terrible
		(currentTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER) ? ((Backbuffer*)currentTarget)->getImageAvailableSemaphore() : VK_NULL_HANDLE,
		VK_NULL_HANDLE
	};

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };

	if (m_waitOnCompute)
	{
		waitForFinishSemaphores[waitSemaphoreCount++] = m_computeFinishedSemaphores[m_currentFrameIdx];
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &currentBuffer;

	submitInfo.signalSemaphoreCount = signalSemaphoreCount;
	submitInfo.pSignalSemaphores = queueFinishedSemaphore;

	submitInfo.waitSemaphoreCount = waitSemaphoreCount;
	submitInfo.pWaitSemaphores = waitForFinishSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	vkResetFences(g_vulkanBackend->m_device, 1, &currentFrame.inFlightFence);

	LLT_VK_CHECK(
		vkQueueSubmit(g_vulkanBackend->m_graphicsQueue.getQueue(), 1, &submitInfo, currentFrame.inFlightFence),
		"Failed to submit draw command to buffer"
	);

	m_waitOnCompute = false;
	m_currentRenderTarget = nullptr;
}

void VulkanBackend::waitOnCompute()
{
	m_waitOnCompute = true;
}

void VulkanBackend::beginCompute()
{
	cauto& currentFrame = m_computeQueues[0].getCurrentFrame();
	VkCommandBuffer currentBuffer = currentFrame.commandBuffer;

	vkWaitForFences(m_device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

	vkResetCommandPool(m_device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	LLT_VK_CHECK(
		vkBeginCommandBuffer(currentBuffer, &commandBufferBeginInfo),
		"Failed to begin recording compute command buffer"
	);
}

void VulkanBackend::endCompute()
{
	cauto& currentFrame = m_computeQueues[0].getCurrentFrame();
	VkCommandBuffer currentBuffer = currentFrame.commandBuffer;

	LLT_VK_CHECK(
		vkEndCommandBuffer(currentBuffer),
		"Failed to record compute command buffer"
	);

	VkSubmitInfo submitInfo = {};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &currentBuffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_computeFinishedSemaphores[m_currentFrameIdx];

	vkResetFences(m_device, 1, &currentFrame.inFlightFence);

	LLT_VK_CHECK(
		vkQueueSubmit(m_computeQueues[0].getQueue(), 1, &submitInfo, currentFrame.inFlightFence),
		"Failed to submit compute command buffer"
	);
}

void VulkanBackend::swapBuffers()
{
	vkWaitForFences(m_device, 1, &m_graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);

	m_backbuffer->swapBuffers();

	m_currentFrameIdx = (m_currentFrameIdx + 1) % mgc::FRAMES_IN_FLIGHT;

	g_shaderBufferManager->resetBufferUsageInFrame();

	m_backbuffer->acquireNextImage();
}

void VulkanBackend::syncStall() const
{
	while (g_platform->getWindowSize() == glm::ivec2(0, 0)) {}
	vkDeviceWaitIdle(m_device);
}

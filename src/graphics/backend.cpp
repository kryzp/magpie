#include "backend.h"
#include "util.h"
#include "texture.h"
#include "backbuffer.h"

#include "../platform.h"

#include "../math/calc.h"

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
		LLT_ERROR("[VULKAN|DEBUG] Validation Layer (SEVERITY: %d) (TYPE: %d): %s", messageSeverity, messageType, pCallbackData->pMessage);
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
		LLT_ERROR("[VULKAN|DEBUG] Unable to get instance extension count.");
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
	: vulkanInstance()
	, device()
	, physicalData()
	, maxMsaaSamples()
	, swapChainImageFormat()
	, vmaAllocator()
	, descriptorPoolManager()
	, descriptorCache()
	, pipelineCache()
	, pipelineLayoutCache()
	, graphicsQueue()
	, computeQueues()
	, transferQueues()
	, pushConstants()
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

#if LLT_DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

	// check for validation layers
	if (debugHasValidationLayerSupport())
	{
		// validation layers confirmed!
		LLT_LOG("[VULKAN|DEBUG] Validation layer support verified.");

		// create the debugging messenger
		debugPopulateDebugUtilsMessengerCreateInfoExt(&debugCreateInfo);

		createInfo.enabledLayerCount = LLT_ARRAY_LENGTH(vkutil::VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = vkutil::VALIDATION_LAYERS;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

		g_debugEnableValidationLayers = true;
	}
	else
	{
		// unfortunately no validation layers, continue on with the program
		LLT_LOG("[VULKAN|DEBUG] No validation layer support.");

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
	if (VkResult result = vkCreateInstance(&createInfo, nullptr, &vulkanInstance); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create instance: %d", result);
	}

#if LLT_DEBUG
	if (VkResult result = debugCreateDebugUtilsMessengerExt(vulkanInstance, &debugCreateInfo, nullptr, &m_debugMessenger); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create debug messenger: %d", result);
	}
#endif // LLT_DEBUG

	// set up all of the core managers of resources
	g_gpuBufferManager       = new GPUBufferMgr();
	g_shaderBufferManager = new ShaderBufferMgr();
	g_textureManager      = new TextureMgr();
	g_shaderManager       = new ShaderMgr();
	g_renderTargetManager = new RenderTargetMgr();

	// finished :D
	LLT_LOG("[VULKAN] Initialized!");
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

	descriptorCache.cleanUp();
	descriptorPoolManager.cleanUp();

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyFence(this->device, graphicsQueue.getFrame(i).inFlightFence, nullptr);
		vkDestroyCommandPool(this->device, graphicsQueue.getFrame(i).commandPool, nullptr);

		vkDestroySemaphore(this->device, m_computeFinishedSemaphores[i], nullptr);

		for (int j = 0; j < computeQueues.size(); j++) {
			vkDestroyFence(this->device, computeQueues[j].getFrame(i).inFlightFence, nullptr);
			vkDestroyCommandPool(this->device, computeQueues[j].getFrame(i).commandPool, nullptr);
		}

		for (int j = 0; j < transferQueues.size(); j++) {
			vkDestroyFence(this->device, transferQueues[j].getFrame(i).inFlightFence, nullptr);
			vkDestroyCommandPool(this->device, transferQueues[j].getFrame(i).commandPool, nullptr);
		}
	}

	delete m_backbuffer;

	vmaDestroyAllocator(vmaAllocator);

	vkDestroyDevice(this->device, nullptr);

#if LLT_DEBUG
	// if we have validation layers then destroy them
	if (g_debugEnableValidationLayers) {
		debugDestroyDebugUtilsMessengerExt(vulkanInstance, m_debugMessenger, nullptr);
		LLT_LOG("[VULKAN|DEBUG] Destroyed validation layers!");
	}
#endif // LLT_DEBUG

	// finished with vulkan, may now safely destroy the vulkan instance
	vkDestroyInstance(vulkanInstance, nullptr);

	// //

	LLT_LOG("[VULKAN] Destroyed!");
}

void VulkanBackend::enumeratePhysicalDevices()
{
	VkSurfaceKHR surface = m_backbuffer->getSurface();

	// get the total devices we have
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);

	// no viable physical device found, exit program
	if (!deviceCount) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to find GPUs with Vulkan support!");
	}

	VkPhysicalDeviceProperties properties = {};
	VkPhysicalDeviceFeatures features = {};

	Vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices.data());

	vkGetPhysicalDeviceProperties(devices[0], &properties);
	vkGetPhysicalDeviceFeatures(devices[0], &features);

	physicalData.device = devices[0];
	physicalData.properties = properties;
	physicalData.features = features;

	// get the actual number of samples we can take
	this->maxMsaaSamples = getMaxUsableSampleCount();

	// try to rank our physical devices and select one accordingly
	bool hasEssentials = false;
	uint32_t usability0 = vkutil::assignPhysicalDeviceUsability(surface, physicalData.device, properties, features, &hasEssentials);

	// select the device of the highest usability
	int i = 1;
	for (; i < deviceCount; i++)
	{
		vkGetPhysicalDeviceProperties(devices[i], &physicalData.properties);
		vkGetPhysicalDeviceFeatures(devices[i], &physicalData.features);

		uint32_t usability1 = vkutil::assignPhysicalDeviceUsability(surface, devices[i], properties, features, &hasEssentials);

		if (usability1 > usability0 && hasEssentials)
		{
			usability0 = usability1;

			physicalData.device = devices[i];
			physicalData.properties = properties;
			physicalData.features = features;

			this->maxMsaaSamples = getMaxUsableSampleCount();
		}
	}

	// still no device with the abilities we need, exit the program
	if (physicalData.device == VK_NULL_HANDLE) {
		LLT_ERROR("[VULKAN|DEBUG] Unable to find a suitable GPU!");
	}

	LLT_LOG("[VULKAN] Selected a suitable GPU: %d", i);
}

void VulkanBackend::createLogicalDevice()
{
	const float QUEUE_PRIORITY = 1.0f;

	Vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	queueCreateInfos.pushBack({
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = graphicsQueue.getFamilyIdx().value(),
		.queueCount = 1,
		.pQueuePriorities = &QUEUE_PRIORITY
	});

	for (auto& computeQueue : computeQueues)
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

	for (auto& transferQueue : transferQueues)
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

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeaturesExt = {};
	dynamicRenderingFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamicRenderingFeaturesExt.dynamicRendering = VK_TRUE;
	dynamicRenderingFeaturesExt.pNext = nullptr;

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
	createInfo.pEnabledFeatures = &physicalData.features;
	createInfo.pNext = &bufferDeviceAddressFeaturesExt;

#if LLT_DEBUG
	// enable the validation layers on the device
	if (g_debugEnableValidationLayers) {
		createInfo.enabledLayerCount = LLT_ARRAY_LENGTH(vkutil::VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = vkutil::VALIDATION_LAYERS;
	}
	LLT_LOG("[VULKAN|DEBUG] Enabled validation layers!");
#endif // LLT_DEBUG

	// create it
	if (VkResult result = vkCreateDevice(physicalData.device, &createInfo, nullptr, &this->device); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create logical device: %d", result);
	}

	// get the device queues for the four core vulkan families
	VkQueue tmpQueue;

	vkGetDeviceQueue(this->device, graphicsQueue.getFamilyIdx().value(), 0, &tmpQueue);
	graphicsQueue.init(tmpQueue);

	for (auto& computeQueue : computeQueues)
	{
		if (!computeQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		vkGetDeviceQueue(this->device, computeQueue.getFamilyIdx().value(), 0, &tmpQueue);
		computeQueue.init(tmpQueue);
	}

	for (auto& transferQueue : transferQueues)
	{
		if (!transferQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		vkGetDeviceQueue(this->device, transferQueue.getFamilyIdx().value(), 0, &tmpQueue);
		transferQueue.init(tmpQueue);
	}

	// init allocator
	createVmaAllocator();

	// print out current device version
	uint32_t version = 0;
	VkResult result = vkEnumerateInstanceVersion(&version);

	if (result == VK_SUCCESS)
	{
		uint32_t major = VK_VERSION_MAJOR(version);
		uint32_t minor = VK_VERSION_MINOR(version);

		printf("[VULKAN] Using version: %d.%d\n", major, minor);
	}

	// get function pointers
#ifdef LLT_MAC_SUPPORT
	vkutil::ext_vkCmdBeginRendering = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR");
	vkutil::ext_vkCmdEndRendering = (PFN_vkCmdEndRendering)vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR");
#endif // LLT_MAC_SUPPORT

	LLT_LOG("[VULKAN] Created logical device!");
}

void VulkanBackend::createVmaAllocator()
{
	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.physicalDevice = physicalData.device;
	allocatorCreateInfo.device = this->device;
	allocatorCreateInfo.instance = this->vulkanInstance;
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	
	if (VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create memory allocator: %d", result);
	}

	LLT_LOG("[VULKAN] Created memory allocator!");
}

void VulkanBackend::createPipelineProcessCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.pNext = nullptr;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = nullptr;

	if (VkResult result = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_pipelineProcessCache); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to process pipeline cache: %d", result);
	}

	LLT_LOG("[VULKAN] Created graphics pipeline process cache!");
}

void VulkanBackend::createComputeResources()
{
	VkSemaphoreCreateInfo computeSemaphoreCreateInfo = {};
	computeSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		if (VkResult result = vkCreateSemaphore(this->device, &computeSemaphoreCreateInfo, nullptr, &m_computeFinishedSemaphores[i]); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN|DEBUG] Failed to create compute finished semaphore: %d", result);
		}
	}

	LLT_LOG("[VULKAN] Created compute resources!");
}

void VulkanBackend::createCommandPools()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	// create a command pool for every frame in flight
	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		createInfo.queueFamilyIndex = graphicsQueue.getFamilyIdx().value();

		if (VkResult result = vkCreateCommandPool(this->device, &createInfo, nullptr, &graphicsQueue.getFrame(i).commandPool); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN|DEBUG] Failed to create graphics command pool: %d", result);
		}

		for (int j = 0; j < computeQueues.size(); j++)
		{
			createInfo.queueFamilyIndex = computeQueues[j].getFamilyIdx().value();

			if (VkResult result = vkCreateCommandPool(this->device, &createInfo, nullptr, &computeQueues[j].getFrame(i).commandPool); result != VK_SUCCESS) {
				LLT_ERROR("[VULKAN|DEBUG] Failed to create compute command pool: %d", result);
			}
		}

		for (int j = 0; j < transferQueues.size(); j++)
		{
			createInfo.queueFamilyIndex = transferQueues[j].getFamilyIdx().value();

			if (VkResult result = vkCreateCommandPool(this->device, &createInfo, nullptr, &transferQueues[j].getFrame(i).commandPool); result != VK_SUCCESS) {
				LLT_ERROR("[VULKAN|DEBUG] Failed to create transfer command pool: %d", result);
			}
		}
	}

	LLT_LOG("[VULKAN] Created command pool!");
}

void VulkanBackend::createCommandBuffers()
{
	// create a command buffer for every frame in flight
	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = graphicsQueue.getFrame(i).commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		if (VkResult result = vkAllocateCommandBuffers(this->device, &commandBufferAllocateInfo, &graphicsQueue.getFrame(i).commandBuffer); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN|DEBUG] Failed to create graphics command buffer: %d", result);
		}

		for (int j = 0; j < computeQueues.size(); j++)
		{
			auto& computeQueue = computeQueues[j];

			commandBufferAllocateInfo.commandPool = computeQueue.getFrame(i).commandPool;

			if (VkResult result = vkAllocateCommandBuffers(this->device, &commandBufferAllocateInfo, &computeQueue.getFrame(i).commandBuffer); result != VK_SUCCESS) {
				LLT_ERROR("[VULKAN|DEBUG] Failed to create compute command buffer: %d", result);
			}
		}

		for (int j = 0; j < transferQueues.size(); j++)
		{
			auto& transferQueue = transferQueues[j];

			commandBufferAllocateInfo.commandPool = transferQueue.getFrame(i).commandPool;

			if (VkResult result = vkAllocateCommandBuffers(this->device, &commandBufferAllocateInfo, &transferQueue.getFrame(i).commandBuffer); result != VK_SUCCESS) {
				LLT_ERROR("[VULKAN|DEBUG] Failed to create transfer command buffer: %d", result);
			}
		}
	}

	LLT_LOG("[VULKAN] Created command buffer!");
}

Backbuffer* VulkanBackend::createBackbuffer()
{
	// make a new backbuffer and create its surface
	Backbuffer* backbuffer = new Backbuffer();
	backbuffer->createSurface();

	m_backbuffer = backbuffer;

	enumeratePhysicalDevices(); // find a physical device

	findQueueFamilies(physicalData.device, m_backbuffer->getSurface());

	createLogicalDevice();
	createPipelineProcessCache();

	createCommandPools();
	createCommandBuffers();

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++) {
		if (VkResult result = vkCreateFence(this->device, &fenceCreateInfo, nullptr, &graphicsQueue.getFrame(i).inFlightFence); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN|DEBUG] Failed to create graphics in flight fence: %d", result);
		}

		for (int j = 0; j < computeQueues.size(); j++) {
			if (VkResult result = vkCreateFence(this->device, &fenceCreateInfo, nullptr, &computeQueues[j].getFrame(i).inFlightFence); result != VK_SUCCESS) {
				LLT_ERROR("[VULKAN|DEBUG] Failed to create compute in flight fence: %d", result);
			}
		}

		for (int j = 0; j < transferQueues.size(); j++) {
			if (VkResult result = vkCreateFence(this->device, &fenceCreateInfo, nullptr, &transferQueues[j].getFrame(i).inFlightFence); result != VK_SUCCESS) {
				LLT_ERROR("[VULKAN|DEBUG] Failed to create transfer in flight fence: %d", result);
			}
		}
	}

	LLT_LOG("[VULKAN] Created in flight fence!");

	createComputeResources();

	descriptorPoolManager.init();

	backbuffer->create();

	// finished!
	return backbuffer;
}

void VulkanBackend::clearPipelineCache()
{
	for (auto& [id, cache] : pipelineCache) {
		vkDestroyPipeline(this->device, cache, nullptr);
	}

	pipelineCache.clear();

	for (auto& [id, cache] : pipelineLayoutCache) {
		vkDestroyPipelineLayout(this->device, cache, nullptr);
	}

	pipelineLayoutCache.clear();

	vkDestroyPipelineCache(this->device, m_pipelineProcessCache, nullptr);
}

VkSampleCountFlagBits VulkanBackend::getMaxUsableSampleCount() const
{
	VkSampleCountFlags counts =
		physicalData.properties.limits.framebufferColorSampleCounts &
		physicalData.properties.limits.framebufferDepthSampleCounts;

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

	LLT_ERROR("[VULKAN|DEBUG] Could not find a maximum usable sample count!");
	return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanBackend::findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	if (!queueFamilyCount) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to find any queue families!");
	}

	Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	Vector<uint32_t> numQueuesFound;
	numQueuesFound.resize(queueFamilyCount);

	bool foundGraphicsQueue = false;

	for (int i = 0; i < queueFamilyCount; i++)
	{
		if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !foundGraphicsQueue)
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			if (presentSupport)
			{
				graphicsQueue.setData(QUEUE_FAMILY_GRAPHICS, i);
				foundGraphicsQueue = true;
				continue;
			}
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && numQueuesFound[i] < queueFamilies[i].queueCount)
		{
			computeQueues.emplaceBack();
			computeQueues.back().setData(QUEUE_FAMILY_COMPUTE, i);
			numQueuesFound[i]++;
			continue;
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && numQueuesFound[i] < queueFamilies[i].queueCount)
		{
			transferQueues.emplaceBack();
			transferQueues.back().setData(QUEUE_FAMILY_TRANSFER, i);
			numQueuesFound[i]++;
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
	pushConstants = params.getPackedConstants();
}

void VulkanBackend::resetPushConstants()
{
	pushConstants.clear();
}

void VulkanBackend::clearDescriptorCacheAndPool()
{
	descriptorCache.clearSetCache();
//	descriptorPoolManager.resetPools();
}

int VulkanBackend::getCurrentFrameIdx() const
{
	return m_currentFrameIdx;
}

GenericRenderTarget* VulkanBackend::getRenderTarget()
{
	return m_currentRenderTarget;
}

const GenericRenderTarget* VulkanBackend::getRenderTarget() const
{
	return m_currentRenderTarget;
}

void VulkanBackend::beginGraphics(GenericRenderTarget* target)
{
	m_currentRenderTarget = target ? target : m_backbuffer;

	auto currentFrame = g_vulkanBackend->graphicsQueue.getCurrentFrame();
	auto currentBuffer = currentFrame.commandBuffer;

	vkWaitForFences(g_vulkanBackend->device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

	vkResetCommandPool(g_vulkanBackend->device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (VkResult result = vkBeginCommandBuffer(currentBuffer, &commandBufferBeginInfo); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to begin recording command buffer: %d", result);
	}

	m_currentRenderTarget->beginGraphics(currentBuffer);

	VkRenderingInfoKHR renderInfo = m_currentRenderTarget->getRenderInfo()->getInfo();

#if LLT_MAC_SUPPORT
	vkutil::ext_vkCmdBeginRendering(currentBuffer, &renderInfo);
#else
	vkCmdBeginRendering(currentBuffer, &renderInfo);
#endif // LLT_MAC_SUPPORT
}

void VulkanBackend::endGraphics()
{
	auto currentFrame = g_vulkanBackend->graphicsQueue.getCurrentFrame();
	auto currentBuffer = currentFrame.commandBuffer;

#if LLT_MAC_SUPPORT
	vkutil::ext_vkCmdEndRendering(currentBuffer);
#else
	vkCmdEndRendering(currentBuffer);
#endif // LLT_MAC_SUPPORT

	m_currentRenderTarget->endGraphics(currentBuffer);

	if (VkResult result = vkEndCommandBuffer(currentBuffer); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to record command buffer: %d", result);
	}

	int signalSemaphoreCount = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? 1 : 0;
	const VkSemaphore* queueFinishedSemaphore = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? &((Backbuffer*)m_currentRenderTarget)->getRenderFinishedSemaphore() : VK_NULL_HANDLE;

	int waitSemaphoreCount = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? 1 : 0;

	VkSemaphore waitForFinishSemaphores[2] = { // todo: this is pretty ugly and probably terrible
		(m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER) ? ((Backbuffer*)m_currentRenderTarget)->getImageAvailableSemaphore() : VK_NULL_HANDLE,
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

	vkResetFences(g_vulkanBackend->device, 1, &currentFrame.inFlightFence);

	if (VkResult result = vkQueueSubmit(g_vulkanBackend->graphicsQueue.getQueue(), 1, &submitInfo, currentFrame.inFlightFence); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to submit draw command to buffer: %d", result);
	}

	m_waitOnCompute = false;
}

void VulkanBackend::waitOnCompute()
{
	m_waitOnCompute = true;
}

void VulkanBackend::beginCompute()
{
	const auto& currentFrame = this->computeQueues[0].getCurrentFrame();
	VkCommandBuffer currentBuffer = currentFrame.commandBuffer;

	vkWaitForFences(this->device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

	vkResetCommandPool(this->device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (VkResult result = vkBeginCommandBuffer(currentBuffer, &commandBufferBeginInfo); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to begin recording compute command buffer: %d", result);
	}
}

void VulkanBackend::endCompute()
{
	const auto& currentFrame = this->computeQueues[0].getCurrentFrame();
	VkCommandBuffer currentBuffer = currentFrame.commandBuffer;

	if (VkResult result = vkEndCommandBuffer(currentBuffer); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to record compute command buffer: %d", result);
	}

	VkSubmitInfo submitInfo = {};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &currentBuffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_computeFinishedSemaphores[m_currentFrameIdx];

	vkResetFences(this->device, 1, &currentFrame.inFlightFence);

	if (VkResult result = vkQueueSubmit(this->computeQueues[0].getQueue(), 1, &submitInfo, currentFrame.inFlightFence); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to submit compute command buffer: %d", result);
	}
}

void VulkanBackend::swapBuffers()
{
	vkWaitForFences(this->device, 1, &graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);

	m_backbuffer->swapBuffers();

	m_currentFrameIdx = (m_currentFrameIdx + 1) % mgc::FRAMES_IN_FLIGHT;

	g_shaderBufferManager->resetBufferUsageInFrame();

	m_backbuffer->acquireNextImage();
}

void VulkanBackend::syncStall() const
{
	while (g_platform->getWindowSize() == glm::ivec2(0, 0)) {}
	vkDeviceWaitIdle(this->device);
}

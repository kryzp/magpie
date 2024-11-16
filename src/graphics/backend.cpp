#include "backend.h"
#include "util.h"
#include "texture.h"
#include "backbuffer.h"

#include "../platform.h"

#include "../math/calc.h"

llt::VulkanBackend* llt::g_vulkanBackend = nullptr;

using namespace llt;

static VkDynamicState DYNAMIC_STATES[] = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR,
	VK_DYNAMIC_STATE_BLEND_CONSTANTS
};

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

	//extensions.pushBack(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME); // MAC SUPPORT
	extensions.pushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	return extensions;
}

VulkanBackend::VulkanBackend(const Config& config)
	: vulkanInstance(VK_NULL_HANDLE)
	, m_vmaAllocator()
	, m_currentRenderPassBuilder()
	, m_descriptorPoolManager()
	, m_imageBoundIdxs()
	, m_imageInfos()
	, m_graphicsShaderStages()
	, m_sampleShadingEnabled(true)
	, m_pushConstants()
	, m_minSampleShading(0.2f)
	, m_blendStateLogicOpEnabled(false)
	, m_blendStateLogicOp(VK_LOGIC_OP_COPY)
	, m_graphicsPipelineCache()
	, m_pipelineLayoutCache()
	, m_computePipelineCache()
	, m_computeShaderStageInfo()
	, m_computeFinishedSemaphores()
	, m_uncertainComputeFinished(false)
	, m_descriptorCache()
	, m_descriptorBuilder()
	, m_descriptorBuilderDirty(true)
	, m_pipelineProcessCache()
	, swapChainImageFormat()
	, m_currentRenderTarget(nullptr)
	, device(VK_NULL_HANDLE)
	, physicalData()
	, m_depthStencilCreateInfo()
	, m_colourBlendAttachmentStates()
	, m_blendConstants{0.0f, 0.0f, 0.0f, 0.0f}
	, maxMsaaSamples(VK_SAMPLE_COUNT_1_BIT)
	, m_viewport()
	, m_scissor()
	, m_currentFrameIdx(0)
	, m_cullMode(VK_CULL_MODE_BACK_BIT)
	, m_currentVertexDescriptor(nullptr)
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
	//createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; // MAC SUPPORT

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

	// destroy core managers in reverse order they were created
	delete g_renderTargetManager;
	delete g_shaderManager;
	delete g_textureManager;
	delete g_gpuBufferManager;

	m_backbuffer->cleanUpTextures();

	// destroy the pipeline cache
	clearPipelineCache();
	vkDestroyPipelineCache(this->device, m_pipelineProcessCache, nullptr);

	m_currentRenderPassBuilder->cleanUp();

	delete g_shaderBufferManager;

	m_descriptorCache.cleanUp();
	m_descriptorPoolManager.cleanUp();

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++) {
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

	vmaDestroyAllocator(m_vmaAllocator);

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
	for (int i = 1; i < deviceCount; i++)
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

	LLT_LOG("[VULKAN] Selected a suitable GPU: %d", physicalData.device);
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
	
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeaturesExt = {};
	bufferDeviceAddressFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferDeviceAddressFeaturesExt.bufferDeviceAddress = VK_TRUE;

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

	LLT_LOG("[VULKAN] Created logical device!");
}

void VulkanBackend::createVmaAllocator()
{
	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.physicalDevice = physicalData.device;
	allocatorCreateInfo.device = this->device;
	allocatorCreateInfo.instance = this->vulkanInstance;
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	
	if (VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator); result != VK_SUCCESS) {
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

VkPipeline VulkanBackend::getComputePipeline()
{
	VkPipelineLayout layout = getPipelineLayout(VK_SHADER_STAGE_COMPUTE_BIT);

	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &m_computeShaderStageInfo);
	hash::combine(&createdPipelineHash, &layout);

	if (m_computePipelineCache.contains(createdPipelineHash)) {
		return m_computePipelineCache[createdPipelineHash];
	}

	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.stage = m_computeShaderStageInfo;

	VkPipeline createdPipeline = VK_NULL_HANDLE;

	if (VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &createdPipeline); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create new compute pipeline: %d", result);
	}

	LLT_LOG("[VULKAN] Created new compute pipeline!");

	m_computePipelineCache.insert(Pair(
		createdPipelineHash,
		createdPipeline
	));

	return createdPipeline;
}

VkPipelineLayout VulkanBackend::getPipelineLayout(VkShaderStageFlagBits stage)
{
	DescriptorBuilder builder = getDescriptorBuilder(stage);
	uint64_t pipelineLayoutHash = builder.hash();

	hash::combine(&pipelineLayoutHash, &stage);

	uint64_t pushConstantCount = m_pushConstants.size();
	hash::combine(&pipelineLayoutHash, &pushConstantCount);

	// check the pipeline cache in-case the needed pipeline already exists
	// (i.e: was created at some point prior)
	if (m_pipelineLayoutCache.contains(pipelineLayoutHash)) {
		return m_pipelineLayoutCache[pipelineLayoutHash];
	}

	// no pipeline found!
	// continue...

	VkDescriptorSetLayout layout = {};
	builder.buildLayout(layout); // get the layout of the pipeline

	VkPushConstantRange pushConstants;
	pushConstants.offset = 0;
	pushConstants.size = m_pushConstants.size();
	pushConstants.stageFlags = stage;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &layout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (pushConstants.size > 0) {
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
	}

	VkPipelineLayout pipelineLayout = {};

	if (VkResult result = vkCreatePipelineLayout(this->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create pipeline layout: %d", result);
	}

	LLT_LOG("[VULKAN] Created new pipeline layout!");

	m_pipelineLayoutCache.insert(Pair(
		pipelineLayoutHash,
		pipelineLayout
	));

	return pipelineLayout;
}

VkPipeline VulkanBackend::getGraphicsPipeline()
{
	// this is essentially a long list of configurations vulkan requires us to give
	// it to fully specify how a graphics pipeline should behave

	LLT_ASSERT(m_currentVertexDescriptor, "[VULKAN|DEBUG] Vertex descriptor must not be null!");
	
	const auto& bindingDescriptions = m_currentVertexDescriptor->getBindingDescriptions();
	const auto& attributeDescriptions = m_currentVertexDescriptor->getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &m_viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &m_scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = m_cullMode;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = m_sampleShadingEnabled ? VK_TRUE : VK_FALSE;
	multisampleStateCreateInfo.minSampleShading = m_minSampleShading;
	multisampleStateCreateInfo.rasterizationSamples = m_currentRenderTarget->getMSAA();
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo = {};
	colourBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendStateCreateInfo.logicOpEnable = m_blendStateLogicOpEnabled ? VK_TRUE : VK_FALSE;
	colourBlendStateCreateInfo.logicOp = m_blendStateLogicOp;
	colourBlendStateCreateInfo.attachmentCount = m_currentRenderPassBuilder->getColourAttachmentCount();
	colourBlendStateCreateInfo.pAttachments = m_colourBlendAttachmentStates.data();
	colourBlendStateCreateInfo.blendConstants[0] = m_blendConstants[0];
	colourBlendStateCreateInfo.blendConstants[1] = m_blendConstants[1];
	colourBlendStateCreateInfo.blendConstants[2] = m_blendConstants[2];
	colourBlendStateCreateInfo.blendConstants[3] = m_blendConstants[3];

	uint64_t createdPipelineHash = 0;

	// now that we have made all of our settings that impact the graphics
	// pipeline, hash them all together
	hash::combine(&createdPipelineHash, &m_depthStencilCreateInfo);
	hash::combine(&createdPipelineHash, m_colourBlendAttachmentStates.data());
	hash::combine(&createdPipelineHash, &m_blendConstants);
	hash::combine(&createdPipelineHash, &rasterizationStateCreateInfo);
	hash::combine(&createdPipelineHash, &inputAssemblyStateCreateInfo);
	hash::combine(&createdPipelineHash, &multisampleStateCreateInfo);

	VkRenderPass pass = m_currentRenderPassBuilder->getRenderPass();

	hash::combine(&createdPipelineHash, &pass);

	for (auto& attrib : attributeDescriptions) {
		hash::combine(&createdPipelineHash, &attrib);
	}

	for (auto& binding : bindingDescriptions) {
		hash::combine(&createdPipelineHash, &binding);
	}

	for (int i = 0; i < m_graphicsShaderStages.size(); i++) {
		hash::combine(&createdPipelineHash, &m_graphicsShaderStages[i]);
	}

	// this hash now uniquely identifies a pipeline
	// if it exists in our cache (because it has been created before)
	// then we return that because it is far cheaper than creating a new pipeline

	if (m_graphicsPipelineCache.contains(createdPipelineHash)) {
		// alright, a pipeline was found!
		return m_graphicsPipelineCache[createdPipelineHash];
	}

	// no such pipeline exists!

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = LLT_ARRAY_LENGTH(DYNAMIC_STATES);
	dynamicStateCreateInfo.pDynamicStates = DYNAMIC_STATES;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pStages = m_graphicsShaderStages.data();
	graphicsPipelineCreateInfo.stageCount = m_graphicsShaderStages.size();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &m_depthStencilCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colourBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = getPipelineLayout(VK_SHADER_STAGE_ALL_GRAPHICS);
	graphicsPipelineCreateInfo.renderPass = m_currentRenderPassBuilder->getRenderPass();
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	VkPipeline createdPipeline = VK_NULL_HANDLE;

	if (VkResult result = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &createdPipeline); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create new graphics pipeline: %d", result);
	}

	LLT_LOG("[VULKAN] Created new graphics pipeline!");

	m_graphicsPipelineCache.insert(Pair(
		createdPipelineHash,
		createdPipeline
	));

	return createdPipeline;
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
	setRenderTarget(backbuffer); // set the backbuffer as the initial render target

	enumeratePhysicalDevices(); // find a physical device

	m_depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	m_depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	m_depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	m_depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	m_depthStencilCreateInfo.minDepthBounds = 0.0f;
	m_depthStencilCreateInfo.maxDepthBounds = 1.0f;
	m_depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
	m_depthStencilCreateInfo.front = {};
	m_depthStencilCreateInfo.back = {};

	for (int i = 0; i < m_colourBlendAttachmentStates.size(); i++)
	{
		m_colourBlendAttachmentStates[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_colourBlendAttachmentStates[i].blendEnable = VK_TRUE;
		m_colourBlendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		m_colourBlendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		m_colourBlendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
		m_colourBlendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		m_colourBlendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_colourBlendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
	}

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

	m_descriptorPoolManager.init();

	backbuffer->create();

	setRenderTarget(m_backbuffer);

	resetViewport();
	resetScissor();

	// finished!
	return backbuffer;
}

void VulkanBackend::markDescriptorDirty()
{
	m_descriptorBuilderDirty = true;
}

void VulkanBackend::resetDescriptorBuilder()
{
	markDescriptorDirty();

	m_descriptorBuilder.reset(
		&m_descriptorPoolManager,
		&m_descriptorCache
	);
}

DescriptorBuilder VulkanBackend::getDescriptorBuilder(VkShaderStageFlagBits stage)
{
	if (!m_descriptorBuilderDirty) {
		return m_descriptorBuilder;
	}

	resetDescriptorBuilder();

	g_shaderBufferManager->bindToDescriptorBuilder(&m_descriptorBuilder, stage);

	for (int i = 0; i < m_imageInfos.size(); i++)
	{
		if (m_imageInfos[i].imageView)
		{
			m_descriptorBuilder.bindImage(
				m_imageBoundIdxs[i],
				&m_imageInfos[i],
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				stage
			);
		}
		else
		{
			break;
		}
	}

	m_descriptorBuilderDirty = false;
	return m_descriptorBuilder;
}

/*
 * Creates a new descriptor set based on our currently bound textures
 * by asking the descriptor builder to build one.
 */
VkDescriptorSet VulkanBackend::getDescriptorSet(VkShaderStageFlagBits stage)
{
	uint64_t descriptorSetHash = 0;

	hash::combine(&descriptorSetHash, &stage);

	for (int i = 0; i < m_imageInfos.size(); i++) {
		if (!m_imageInfos[i].imageView) {
			break;
		} else {
			hash::combine(&descriptorSetHash, &m_imageInfos[i]);
		}
	}

	VkDescriptorSet descriptorSet = {};
	VkDescriptorSetLayout descriptorSetLayout = {};

	DescriptorBuilder builder = getDescriptorBuilder(stage);
	builder.build(descriptorSet, descriptorSetLayout, descriptorSetHash);

	return descriptorSet;
}

void VulkanBackend::clearPipelineCache()
{
	for (auto& [id, cache] : m_graphicsPipelineCache) {
		vkDestroyPipeline(this->device, cache, nullptr);
	}

	m_graphicsPipelineCache.clear();

	for (auto& [id, cache] : m_computePipelineCache) {
		vkDestroyPipeline(this->device, cache, nullptr);
	}

	m_computePipelineCache.clear();

	for (auto& [id, cache] : m_pipelineLayoutCache) {
		vkDestroyPipelineLayout(this->device, cache, nullptr);
	}

	m_pipelineLayoutCache.clear();
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

void VulkanBackend::setSampleShading(bool enabled, float minSampleShading)
{
	m_sampleShadingEnabled = enabled;
	m_minSampleShading = minSampleShading;
}

void VulkanBackend::setCullMode(VkCullModeFlagBits cull)
{
	m_cullMode = cull;
}

void VulkanBackend::setTexture(uint32_t bindIdx, const Texture* texture, TextureSampler* sampler)
{
	int textureIdx = 0;
	for (; textureIdx < m_imageInfos.size(); textureIdx++) {
		if (!m_imageInfos[textureIdx].imageView || m_imageBoundIdxs[textureIdx] == bindIdx) {
			break;
		}
	}

	if (texture)
	{
		VkImageView vkImageView = texture->getImageView();

		if (m_imageInfos[textureIdx].imageView != vkImageView)
		{
			m_imageBoundIdxs[textureIdx] = bindIdx;

			m_imageInfos[textureIdx].imageView = vkImageView;

			if (texture->isDepthTexture())
			{
				m_imageInfos[textureIdx].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
			else
			{
				m_imageInfos[textureIdx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			markDescriptorDirty();
		}
	}
	else
	{
		m_imageInfos[textureIdx].imageView = VK_NULL_HANDLE;
	}

	if (sampler)
	{
		VkSampler vkSampler = sampler->bind(device, physicalData.properties, 4);

		if (m_imageInfos[textureIdx].sampler != vkSampler)
		{
			m_imageInfos[textureIdx].sampler = vkSampler;
			markDescriptorDirty();
		}
	}
	else
	{
		m_imageInfos[textureIdx].sampler = nullptr;
	}
}

void VulkanBackend::bindShader(const ShaderProgram* shader)
{
	if (!shader) {
		return;
	}

	switch (shader->type)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			m_graphicsShaderStages[0] = shader->getShaderStageCreateInfo();
			break;

		case VK_SHADER_STAGE_FRAGMENT_BIT:
			m_graphicsShaderStages[1] = shader->getShaderStageCreateInfo();
			break;

		case VK_SHADER_STAGE_GEOMETRY_BIT:
			LLT_ERROR("[VULKAN|DEBUG] Geometry shaders not yet implemented!!");
			break;

		case VK_SHADER_STAGE_COMPUTE_BIT:
			m_computeShaderStageInfo = shader->getShaderStageCreateInfo();
			break;

		default:
			LLT_ERROR("[VULKAN|DEBUG] Unrecognised shader type being bound: %d", shader->type);
			break;
	}
}

void VulkanBackend::setPushConstants(ShaderParameters& params)
{
	m_pushConstants = params.getPackedConstants();
}

void VulkanBackend::resetPushConstants()
{
	m_pushConstants.clear();
}

void VulkanBackend::setVertexDescriptor(const VertexDescriptor& descriptor)
{
	m_currentVertexDescriptor = &descriptor;
}

void VulkanBackend::clearDescriptorCacheAndPool()
{
	m_descriptorCache.clearSetCache();
//	m_descriptorPoolManager.resetPools();
}

int VulkanBackend::getCurrentFrameIdx() const
{
	return m_currentFrameIdx;
}

void VulkanBackend::beginCompute()
{
	markDescriptorDirty();

	const auto& currentFrame = computeQueues[0].getCurrentFrame();
	VkCommandBuffer currentBuffer = currentFrame.commandBuffer;

	vkWaitForFences(this->device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

	vkResetCommandPool(this->device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (VkResult result = vkBeginCommandBuffer(currentBuffer, &commandBufferBeginInfo); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to begin recording compute command buffer: %d", result);
	}
}

void VulkanBackend::dispatchCompute(int gcX, int gcY, int gcZ)
{
	const auto& currentFrame = computeQueues[0].getCurrentFrame();
	VkCommandBuffer currentBuffer = currentFrame.commandBuffer;

	VkPipeline pipeline = getComputePipeline();
	VkPipelineLayout pipelineLayout = getPipelineLayout(VK_SHADER_STAGE_COMPUTE_BIT);
	VkDescriptorSet descriptorSet = getDescriptorSet(VK_SHADER_STAGE_COMPUTE_BIT);

	if (m_pushConstants.size() > 0)
	{
		vkCmdPushConstants(
			currentBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			m_pushConstants.size(),
			m_pushConstants.data()
		);
	}

	Vector<uint32_t> dynamicOffsets = g_shaderBufferManager->getDynamicOffsets();

	LLT_LOG("%d %d", dynamicOffsets[0], dynamicOffsets[1]);

	vkCmdBindDescriptorSets(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout,
		0,
		1, &descriptorSet,
		dynamicOffsets.size(),
		dynamicOffsets.data()
	);

	vkCmdBindPipeline(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipeline
	);

	vkCmdDispatch(
		currentBuffer,
		gcX, gcY, gcZ
	);
}

void VulkanBackend::endCompute()
{
	const auto& currentFrame = computeQueues[0].getCurrentFrame();
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

	if (VkResult result = vkQueueSubmit(computeQueues[0].getQueue(), 1, &submitInfo, currentFrame.inFlightFence); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to submit compute command buffer: %d", result);
	}

	g_shaderBufferManager->unbindAll();
}

void VulkanBackend::syncComputeWithNextRender()
{
	m_uncertainComputeFinished = true;
}

void VulkanBackend::beginRender()
{
	markDescriptorDirty();

	const auto& currentFrame = graphicsQueue.getCurrentFrame();
	VkCommandBuffer currentBuffer = currentFrame.commandBuffer;

	vkWaitForFences(this->device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

	vkResetCommandPool(this->device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (VkResult result = vkBeginCommandBuffer(currentBuffer, &commandBufferBeginInfo); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to begin recording command buffer: %d", result);
	}

	VkRenderPassBeginInfo renderPassBeginInfo = m_currentRenderPassBuilder->buildBeginInfo();

	vkCmdBeginRenderPass(currentBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanBackend::render(const RenderOp& op)
{
	VkCommandBuffer currentBuffer = graphicsQueue.getCurrentFrame().commandBuffer;

	vkCmdSetViewport(currentBuffer, 0, 1, &m_viewport);
	vkCmdSetScissor(currentBuffer, 0, 1, &m_scissor);

	const auto& vertexBuffer = op.meshData.vBuffer->getBuffer();
	const auto& indexBuffer  = op.meshData.iBuffer->getBuffer();

	VkPipeline pipeline = getGraphicsPipeline();
	VkPipelineLayout pipelineLayout = getPipelineLayout(VK_SHADER_STAGE_ALL_GRAPHICS);
	VkDescriptorSet descriptorSet = getDescriptorSet(VK_SHADER_STAGE_ALL_GRAPHICS);

	VkBuffer vertexBuffers[2] = { vertexBuffer, VK_NULL_HANDLE };

	if (op.instanceData.buffer)
	{
		vertexBuffers[1] = op.instanceData.buffer->getBuffer();
	}

	VkDeviceSize offsets[] = { 0, 0 };

	const auto& bindings = m_currentVertexDescriptor->getBindingDescriptions();

	for (int i = 0; i < bindings.size(); i++)
	{
		vkCmdBindVertexBuffers(currentBuffer, bindings[i].binding, 1, &vertexBuffers[i], offsets);
	}

	vkCmdBindIndexBuffer(currentBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	if (m_pushConstants.size() > 0)
	{
		vkCmdPushConstants(
			currentBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			0,
			m_pushConstants.size(),
			m_pushConstants.data()
		);
	}

	Vector<uint32_t> dynamicOffsets = g_shaderBufferManager->getDynamicOffsets();

	vkCmdBindDescriptorSets(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1, &descriptorSet,
		dynamicOffsets.size(),
		dynamicOffsets.data()
	);

	vkCmdBindPipeline(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline
	);

	if (op.indirectData.buffer)
	{
		// todo: start using vkCmdDrawIndirectCount
		vkCmdDrawIndexedIndirect(
			currentBuffer,
			op.indirectData.buffer->getBuffer(),
			op.indirectData.offset,
			op.indirectData.drawCount,
			sizeof(VkDrawIndexedIndirectCommand)
		);
	}
	else
	{
		vkCmdDrawIndexed(
			currentBuffer,
			op.meshData.nIndices,
			op.instanceData.instanceCount,
			0,
			0,
			op.instanceData.firstInstance
		);
	}
}

void VulkanBackend::endRender()
{
	const auto& currentFrame = graphicsQueue.getCurrentFrame();
	VkCommandBuffer currentBuffer = currentFrame.commandBuffer;

	vkCmdEndRenderPass(currentBuffer);

	if (VkResult result = vkEndCommandBuffer(currentBuffer); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to record command buffer: %d", result);
	}

	int signalSemaphoreCount = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? 1 : 0;
	const VkSemaphore* queueFinishedSemaphore = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? &m_backbuffer->getRenderFinishedSemaphore() : VK_NULL_HANDLE;

	int waitSemaphoreCount = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? 1 : 0;

	VkSemaphore waitForFinishSemaphores[2] = { // todo: this is pretty ugly and probably terrible
		(m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER) ? m_backbuffer->getImageAvailableSemaphore() : VK_NULL_HANDLE,
		m_computeFinishedSemaphores[m_currentFrameIdx]
	};

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };

	if (m_uncertainComputeFinished)
	{
		waitForFinishSemaphores[waitSemaphoreCount] = m_computeFinishedSemaphores[m_currentFrameIdx];
		waitSemaphoreCount++;
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

	vkResetFences(this->device, 1, &currentFrame.inFlightFence);

	if (VkResult result = vkQueueSubmit(graphicsQueue.getQueue(), 1, &submitInfo, graphicsQueue.getCurrentFrame().inFlightFence); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to submit draw command to buffer: %d", result);
	}

	m_uncertainComputeFinished = false;

	g_shaderBufferManager->unbindAll();
}

void VulkanBackend::swapBuffers()
{
	vkWaitForFences(this->device, 1, &graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);

	resetDescriptorBuilder();

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

void VulkanBackend::setViewport(const RectF &rect)
{
	m_viewport = { rect.x, rect.y, rect.w, rect.h };
}

void VulkanBackend::setScissor(const RectI& rect)
{
	m_scissor = { rect.x, rect.y, (uint32_t)rect.w, (uint32_t)rect.h };
}

void VulkanBackend::resetViewport()
{
	// we have to flip the viewport to ensure y+ is up!

	m_viewport.x = 0.0f;
	m_viewport.y = (float)m_currentRenderPassBuilder->getHeight();
	m_viewport.width = (float)m_currentRenderPassBuilder->getWidth();
	m_viewport.height = -(float)m_currentRenderPassBuilder->getHeight();
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
}

void VulkanBackend::resetScissor()
{
	m_scissor.offset = { 0, 0 };
	m_scissor.extent = { m_currentRenderPassBuilder->getWidth(), m_currentRenderPassBuilder->getHeight() };
}

void VulkanBackend::setRenderTarget(GenericRenderTarget* target)
{
	m_currentRenderTarget = target;
	m_currentRenderPassBuilder = m_currentRenderTarget->getRenderPassBuilder();
}

void VulkanBackend::setDepthOp(VkCompareOp op)
{
	m_depthStencilCreateInfo.depthCompareOp = op;
}

void VulkanBackend::setDepthTest(bool enabled)
{
	m_depthStencilCreateInfo.depthTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

void VulkanBackend::setDepthWrite(bool enabled)
{
	m_depthStencilCreateInfo.depthWriteEnable = enabled ? VK_TRUE : VK_FALSE;
}

void VulkanBackend::setDepthBounds(float min, float max)
{
	m_depthStencilCreateInfo.minDepthBounds = min;
	m_depthStencilCreateInfo.maxDepthBounds = max;
}

void VulkanBackend::setDepthStencilTest(bool enabled)
{
	m_depthStencilCreateInfo.stencilTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

void VulkanBackend::toggleBlending(bool enabled)
{
	for (int i = 0; i < m_colourBlendAttachmentStates.size(); i++)
	{
		m_colourBlendAttachmentStates[i].blendEnable = enabled ? VK_TRUE : VK_FALSE;
	}
}

void VulkanBackend::setBlendWriteMask(bool r, bool g, bool b, bool a)
{
	for (int i = 0; i < m_colourBlendAttachmentStates.size(); i++)
	{
		m_colourBlendAttachmentStates[i].colorWriteMask = 0;

		if (r) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (g) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (b) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (a) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
	}
}

void VulkanBackend::setBlendColour(const Blend& blend)
{
	for (int i = 0; i < m_colourBlendAttachmentStates.size(); i++)
	{
		m_colourBlendAttachmentStates[i].colorBlendOp = blend.op;
		m_colourBlendAttachmentStates[i].srcColorBlendFactor = blend.src;
		m_colourBlendAttachmentStates[i].dstColorBlendFactor = blend.dst;
	}
}

void VulkanBackend::setBlendAlpha(const Blend& blend)
{
	for (int i = 0; i < m_colourBlendAttachmentStates.size(); i++)
	{
		m_colourBlendAttachmentStates[i].alphaBlendOp = blend.op;
		m_colourBlendAttachmentStates[i].srcAlphaBlendFactor = blend.src;
		m_colourBlendAttachmentStates[i].dstAlphaBlendFactor = blend.dst;
	}
}

void VulkanBackend::setBlendConstants(float r, float g, float b, float a)
{
	m_blendConstants[0] = r;
	m_blendConstants[1] = g;
	m_blendConstants[2] = b;
	m_blendConstants[3] = a;
}

void VulkanBackend::getBlendConstants(float* constants)
{
	mem::copy(constants, m_blendConstants, sizeof(float) * 4);
}

void VulkanBackend::setBlendOp(bool enabled, VkLogicOp op)
{
	m_blendStateLogicOpEnabled = enabled;
	m_blendStateLogicOp = op;
}

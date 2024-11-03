#include "backend.h"
#include "util.h"
#include "texture.h"
#include "backbuffer.h"
#include "vertex.h"

#include "../system_backend.h"

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
		bool has_layer = false;
		const char* layer_name_0 = vkutil::VALIDATION_LAYERS[i];

		for (int j = 0; j < layerCount; j++)
		{
			const char* layer_name_1 = availableLayers[j].layerName;

			if (cstr::compare(layer_name_0, layer_name_1) == 0) {
				has_layer = true;
				break;
			}
		}

		if (!has_layer) {
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
	const VkDebugUtilsMessengerCreateInfoEXT* create_info,
	const VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* debug_messenger
)
{
	auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn) {
		return fn(instance, create_info, allocator, debug_messenger);
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
	uint32_t ext_count = 0;
	const char* const* names = g_systemBackend->vkGetInstanceExtensions(&ext_count);

	if (!names) {
		LLT_ERROR("[VULKAN|DEBUG] Unable to get instance extension count.");
	}

	Vector<const char*> extensions(ext_count);

	for (int i = 0; i < ext_count; i++) {
		extensions[i] = names[i];
	}

#if LLT_DEBUG
	extensions.pushBack(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensions.pushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // LLT_DEBUG

	//extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	extensions.pushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	return extensions;
}

/*
 * Returns the Vulkan representation of the contents of a vertex.
 */
static Array<VkVertexInputAttributeDescription, 4> getVertexAttributeDescription()
{
	Array<VkVertexInputAttributeDescription, 4> result = {};

	// position
	result[0].binding = 0;
	result[0].location = 0;
	result[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	result[0].offset = offsetof(Vertex, pos);

	// uv
	result[1].binding = 0;
	result[1].location = 1;
	result[1].format = VK_FORMAT_R32G32_SFLOAT;
	result[1].offset = offsetof(Vertex, uv);

	// colour
	result[2].binding = 0;
	result[2].location = 2;
	result[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	result[2].offset = offsetof(Vertex, col);

	// normal
	result[3].binding = 0;
	result[3].location = 3;
	result[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	result[3].offset = offsetof(Vertex, norm);

	return result;
}

/*
 * Returns the binding description of a vertex.
 */
static VkVertexInputBindingDescription getVertexBindingDescription()
{
	return {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
}

VulkanBackend::VulkanBackend(const Config& config)
	: vulkanInstance(VK_NULL_HANDLE)
	, m_currentRenderPassBuilder()
	, m_descriptorPoolManager()
	, m_imageInfos()
	, m_shaderStages()
	, m_sampleShadingEnabled(true)
	, m_uboManager()
	, m_pushConstants()
	, m_minSampleShading(0.2f)
	, m_blendStateLogicOpEnabled(false)
	, m_blendStateLogicOp(VK_LOGIC_OP_COPY)
	, m_pipelineCache()
	, m_pipelineLayoutCache()
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
	, msaaSamples(VK_SAMPLE_COUNT_1_BIT)
	, m_viewport()
	, m_scissor()
	, m_cullMode(VK_CULL_MODE_BACK_BIT)
//	, m_current_shader_parameters()
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
	//create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; // molten vk (macos support)

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
	g_bufferManager       = new BufferMgr();
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
	delete g_bufferManager;

	m_backbuffer->cleanUpTextures();

	// destroy the pipeline cache
	clearPipelineCache();
	vkDestroyPipelineCache(this->device, m_pipelineProcessCache, nullptr);

	m_currentRenderPassBuilder->cleanUp();

	m_uboManager.cleanUp(); // ubo data

	m_descriptorCache.cleanUp();
	m_descriptorPoolManager.cleanUp();

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++) {
		vkDestroyFence(this->device, graphicsQueue.getFrame(i).inFlightFence, nullptr);
		vkDestroyCommandPool(this->device, graphicsQueue.getFrame(i).commandPool, nullptr);
	}

	delete m_backbuffer;

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
	this->msaaSamples = getMaxUsableSampleCount();

	// try to rank our physical devices and select one accordingly
	bool has_essentials = false;
	uint32_t usability0 = vkutil::assignPhysicalDeviceUsability(surface, physicalData.device, properties, features, &has_essentials);

	// select the device of the highest usability
	for (int i = 1; i < deviceCount; i++)
	{
		vkGetPhysicalDeviceProperties(devices[i], &physicalData.properties);
		vkGetPhysicalDeviceFeatures(devices[i], &physicalData.features);

		uint32_t usability1 = vkutil::assignPhysicalDeviceUsability(surface, devices[i], properties, features, &has_essentials);

		if (usability1 > usability0 && has_essentials)
		{
			usability0 = usability1;

			physicalData.device = devices[i];
			physicalData.properties = properties;
			physicalData.features = features;

			this->msaaSamples = getMaxUsableSampleCount();
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

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.ppEnabledExtensionNames = vkutil::DEVICE_EXTENSIONS;
	createInfo.ppEnabledExtensionNames = vkutil::DEVICE_EXTENSIONS;
	createInfo.enabledExtensionCount = LLT_ARRAY_LENGTH(vkutil::DEVICE_EXTENSIONS);
	createInfo.pEnabledFeatures = &physicalData.features;

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

	LLT_LOG("[VULKAN] Created a logical device!");
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

VkPipelineLayout VulkanBackend::getGraphicsPipelineLayout()
{
	DescriptorBuilder builder = getDescriptorBuilder();
	uint64_t pipelineLayoutHash = builder.hash();

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
	pushConstants.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

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

	// create it
	if (VkResult result = vkCreatePipelineLayout(this->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create pipeline layout: %d", result);
	}

	LLT_LOG("[VULKAN] Created new graphics pipeline layout!");

	// now that it is created, cache it
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

	VkVertexInputBindingDescription bindingDescription = getVertexBindingDescription();
	Array<VkVertexInputAttributeDescription, 4> attribDescription = getVertexAttributeDescription();

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = attribDescription.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attribDescription.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = m_viewport.width;
	viewport.height = m_viewport.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { (uint32_t)m_viewport.width, (uint32_t)m_viewport.height };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

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
	multisampleStateCreateInfo.rasterizationSamples = (VkSampleCountFlagBits)m_currentRenderTarget->getMSAA();
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

	for (int i = 0; i < attribDescription.size(); i++) {
		hash::combine(&createdPipelineHash, &attribDescription[i]);
	}

	hash::combine(&createdPipelineHash, &bindingDescription);

	for (int i = 0; i < m_shaderStages.size(); i++) {
		hash::combine(&createdPipelineHash, &m_shaderStages[i]);
	}

	// this hash now uniquely identifies a pipeline
	// if it exists in our cache (because it has been created before)
	// then we return that because it is far cheaper than creating a new pipeline

	if (m_pipelineCache.contains(createdPipelineHash)) {
		// alright, a pipeline was found!
		return m_pipelineCache[createdPipelineHash];
	}

	// no such pipeline exists!

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = LLT_ARRAY_LENGTH(DYNAMIC_STATES);
	dynamicStateCreateInfo.pDynamicStates = DYNAMIC_STATES;

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pStages = m_shaderStages.data();
	graphicsPipelineCreateInfo.stageCount = m_shaderStages.size();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &m_depthStencilCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colourBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = getGraphicsPipelineLayout();
	graphicsPipelineCreateInfo.renderPass = m_currentRenderPassBuilder->getRenderPass();
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	VkPipeline createdPipeline = VK_NULL_HANDLE;

	// create it
	if (VkResult result = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &createdPipeline); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create new graphics pipeline: %d", result);
	}

	LLT_LOG("[VULKAN] Created new graphics pipeline!");

	// cache it
	m_pipelineCache.insert(Pair(
		createdPipelineHash,
		createdPipeline
	));

	// return it
	return createdPipeline;
}

void VulkanBackend::createCommandPools(uint32_t graphics_family_idx)
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = graphics_family_idx;

	// create a command pool for every frame in flight
	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++) {
		if (VkResult result = vkCreateCommandPool(this->device, &createInfo, nullptr, &graphicsQueue.getFrame(i).commandPool); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN|DEBUG] Failed to create command pools: %d", result);
		}
	}

	LLT_LOG("[VULKAN] Created command pool!");
}

void VulkanBackend::createCommandBuffers()
{
	// create a command buffer for every frame in flight
	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.commandPool = graphicsQueue.getFrame(i).commandPool;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandBufferCount = 1;

		if (VkResult result = vkAllocateCommandBuffers(this->device, &command_buffer_allocate_info, &graphicsQueue.getFrame(i).commandBuffer); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN|DEBUG] Failed to create command buffers: %d", result);
		}
	}

	LLT_LOG("[VULKAN] Created command buffer!");
}

Backbuffer* VulkanBackend::createBackbuffer()
{
	// make a new backbuffer and create its surface
	Backbuffer* backbuffer = new Backbuffer();
	backbuffer->create_surface();

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

	createCommandPools(graphicsQueue.getFamilyIdx().value());
	createCommandBuffers();

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++) {
		if (VkResult result = vkCreateFence(this->device, &fenceCreateInfo, nullptr, &graphicsQueue.getFrame(i).inFlightFence); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN|DEBUG] Failed to create in flight fence: %d", result);
		}
	}

	LLT_LOG("[VULKAN] Created in flight fence!");

	// start off with 256kB of shader memory for each frame
	// this will increase if it isn't enough, however it is a good baseline.
	m_uboManager.init(KILOBYTES(256) * mgc::FRAMES_IN_FLIGHT);

	m_descriptorPoolManager.init();

	backbuffer->create();

	setRenderTarget(m_backbuffer);

	// set our initial viewport
	m_viewport.x = 0.0f;
	m_viewport.y = 0.0f;
	m_viewport.width = (float)m_currentRenderPassBuilder->getWidth();
	m_viewport.height = (float)m_currentRenderPassBuilder->getHeight();
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	// set the initial scissor
	m_scissor.offset = { 0, 0 };
	m_scissor.extent = { m_currentRenderPassBuilder->getWidth(), m_currentRenderPassBuilder->getHeight() };

	// finished!
	return backbuffer;
}

void VulkanBackend::resetDescriptorBuilder()
{
	m_descriptorBuilderDirty = true;

	m_descriptorBuilder.reset(
		&m_descriptorPoolManager,
		&m_descriptorCache
	);
}

DescriptorBuilder VulkanBackend::getDescriptorBuilder()
{
	// check if our descriptor builder has been modified at all (i.e: dirty)
	// no point in building a new one if it hasn't changed
	if (!m_descriptorBuilderDirty) {
		return m_descriptorBuilder;
	}

	uint32_t nUboDescriptors = (uint32_t)m_uboManager.getDescriptorCount();

	int nTextures = 0;
	for (; nTextures < m_imageInfos.size(); nTextures++) {
		if (!m_imageInfos[nTextures].imageView) {
			break;
		}
	}

	resetDescriptorBuilder();

	// bind ubos
	for (int j = 0; j < nUboDescriptors; j++) {
		m_descriptorBuilder.bindBuffer(
			j,
			&m_uboManager.getDescriptor(j),
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			VK_SHADER_STAGE_ALL_GRAPHICS
		);
	}

	// bind samplers
	for (int i = 0; i < nTextures; i++) {
		m_descriptorBuilder.bindImage(
			nUboDescriptors + i,
			&m_imageInfos[i],
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_ALL_GRAPHICS
		);
	}

	// finished!
	m_descriptorBuilderDirty = false;
	return m_descriptorBuilder;
}

/*
 * Creates a new descriptor set based on our currently bound textures
 * by asking the descriptor builder to build one.
 */
VkDescriptorSet VulkanBackend::getDescriptorSet()
{
	uint64_t descriptorSetHash = 0;

	for (int i = 0; i < m_imageInfos.size(); i++) {
		if (!m_imageInfos[i].imageView) {
			break;
		} else {
			hash::combine(&descriptorSetHash, &m_imageInfos[i]);
		}
	}

	VkDescriptorSet descriptorSet = {};
	VkDescriptorSetLayout descriptorSetLayout = {};

	DescriptorBuilder builder = getDescriptorBuilder();
	builder.build(descriptorSet, descriptorSetLayout, descriptorSetHash);

	return descriptorSet;
}

void VulkanBackend::clearPipelineCache()
{
	for (auto& [id, cache] : m_pipelineCache) {
		vkDestroyPipeline(this->device, cache, nullptr);
	}

	m_pipelineCache.clear();

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
		LLT_ERROR("[VULKAN:UTIL|DEBUG] Failed to find any queue families!");
	}

	Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (int i = 0; i < queueFamilyCount; i++)
	{
		if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			if (presentSupport)
			{
				graphicsQueue.setData(QUEUE_FAMILY_PRESENT_GRAPHICS, i);
				continue;
			}
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			computeQueues.emplaceBack();
			computeQueues.back().setData(QUEUE_FAMILY_COMPUTE, i);
			continue;
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)) {
			transferQueues.emplaceBack();
			transferQueues.back().setData(QUEUE_FAMILY_TRANSFER, i);
			continue;
		}
	}
}

void VulkanBackend::onWindowResize(int width, int height)
{
	m_backbuffer->onWindowResize(width, height);
}

void VulkanBackend::setSampleShading(bool enabled, float min_sample_shading)
{
	m_sampleShadingEnabled = enabled;
	m_minSampleShading = min_sample_shading;
}

void VulkanBackend::setCullMode(VkCullModeFlagBits cull)
{
	m_cullMode = cull;
}

void VulkanBackend::setTexture(uint32_t idx, const Texture* texture)
{
	// if our texture is null then interpret that as unbinding the texture
	// so set the image view at that index to be a null handle.

	if (!texture) {
		m_imageInfos[idx].imageView = VK_NULL_HANDLE;
		return;
	}

	VkImageView vkImageView = texture->getImageView();
	m_imageInfos[idx].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	if (m_imageInfos[idx].imageView != vkImageView)
	{
		m_imageInfos[idx].imageView = vkImageView;
		m_descriptorBuilderDirty = true;
	}
}

void VulkanBackend::setSampler(uint32_t idx, TextureSampler* sampler)
{
	if (!sampler) {
		m_imageInfos[idx].sampler = nullptr;
		return;
	}

	VkSampler vkSampler = sampler->bind(device, physicalData.properties, 4);

	if (m_imageInfos[idx].sampler != vkSampler)
	{
		m_imageInfos[idx].sampler = vkSampler;
		m_descriptorBuilderDirty = true;
	}
}

void VulkanBackend::bindShader(const ShaderProgram* shader)
{
	if (!shader) {
		return;
	}

	int idx = 0;

	switch (shader->type)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			idx = 0;
			break;

		case VK_SHADER_STAGE_FRAGMENT_BIT:
			idx = 1;
			break;

		case VK_SHADER_STAGE_GEOMETRY_BIT:
			idx = 2;
			break;

		case VK_SHADER_STAGE_COMPUTE_BIT:
			idx = 3;
			break;
	}

	m_shaderStages[idx] = shader->getShaderStageCreateInfo();
}

void VulkanBackend::bindShaderParams(VkShaderStageFlagBits shaderType, ShaderParameters& params)
{
	ShaderParameters::PackedData packedConstants = params.getPackedConstants();

	if (packedConstants.size() <= 0) {
		return;
	}

	bool modified = false;
	m_uboManager.push_data(shaderType, packedConstants.data(), packedConstants.size(), &modified);

	if (modified) {
		m_descriptorBuilderDirty = true;
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

void VulkanBackend::clearDescriptorSetAndPool()
{
	m_descriptorCache.clearSetCache();
//	m_descriptor_pool_mgr.reset_pools();
}

void VulkanBackend::beginRender()
{
	vkWaitForFences(this->device, 1, &graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);

	vkResetCommandPool(this->device, graphicsQueue.getCurrentFrame().commandPool, 0);
	VkCommandBuffer currentBuffer = graphicsQueue.getCurrentFrame().commandBuffer;

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

	VkViewport viewport = {};

	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_currentRenderPassBuilder->getWidth();
	viewport.height = (float)m_currentRenderPassBuilder->getHeight();

	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};

	scissor.offset = { 0, 0 };
	scissor.extent = { m_currentRenderPassBuilder->getWidth(), m_currentRenderPassBuilder->getHeight() };

	vkCmdSetViewport(currentBuffer, 0, 1, &viewport);
	vkCmdSetScissor(currentBuffer, 0, 1, &scissor);

//	vkCmdSetViewport(current_buffer, 0, 1, &m_viewport);
//	vkCmdSetScissor(current_buffer, 0, 1, &m_scissor);

	const auto& vertexBuffer = op.vertexData.buffer->getBuffer();
	const auto& indexBuffer  = op.indexData.buffer->getBuffer();

	VkBuffer vertex_buffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(currentBuffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(currentBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	VkPipeline pipeline = getGraphicsPipeline();
	VkPipelineLayout pipelineLayout = getGraphicsPipelineLayout();
	VkDescriptorSet descriptorSet = getDescriptorSet();

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

	int dynamicOffsetCount = m_uboManager.getDynamicOffsetCount();
	const auto& dynamicOffsets = m_uboManager.getDynamicOffsets();

	vkCmdBindDescriptorSets(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1, &descriptorSet,
		dynamicOffsetCount,
		dynamicOffsets.data()
	);

	vkCmdBindPipeline(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline
	);

	vkCmdDrawIndexed(
		currentBuffer,
		op.indexData.indices->size(),
		1,
		0,
		0,
		0
	);
}

void VulkanBackend::endRender()
{
	VkCommandBuffer currentBuffer = graphicsQueue.getCurrentFrame().commandBuffer;

	vkCmdEndRenderPass(currentBuffer);

	if (VkResult result = vkEndCommandBuffer(currentBuffer); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to record command buffer: %d", result);
	}

	const VkSemaphore* waitForFinishSemaphore = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? &m_backbuffer->getImageAvailableSemaphore() : nullptr;
	const VkSemaphore* queueFinishedSemaphore = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? &m_backbuffer->getRenderFinishedSemaphore() : nullptr;
	int semaphoreCount = m_currentRenderTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER ? 1 : 0;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = semaphoreCount;
	submitInfo.pSignalSemaphores = queueFinishedSemaphore;
	submitInfo.signalSemaphoreCount = semaphoreCount;
	submitInfo.pWaitSemaphores = waitForFinishSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &currentBuffer;

	vkResetFences(this->device, 1, &graphicsQueue.getCurrentFrame().inFlightFence);

	if (VkResult result = vkQueueSubmit(graphicsQueue.getQueue(), 1, &submitInfo, graphicsQueue.getCurrentFrame().inFlightFence); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to submit draw command to buffer: %d", result);
	}
}

void VulkanBackend::swapBuffers()
{
	vkWaitForFences(this->device, 1, &graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);

	resetDescriptorBuilder();

	m_backbuffer->swapBuffers();

	graphicsQueue.nextFrame();

	m_uboManager.resetUboUsageInFrame();

	m_backbuffer->aquireNextImage();
}

void VulkanBackend::syncStall() const
{
	while (g_systemBackend->getWindowSize() == glm::ivec2(0, 0)) {}
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

void VulkanBackend::setRenderTarget(GenericRenderTarget* target)
{
	m_currentRenderTarget = target;

	if (target->getType() == RENDER_TARGET_TYPE_TEXTURE) {
		m_currentRenderPassBuilder = ((RenderTarget*)m_currentRenderTarget)->getRenderPassBuilder();
	} else if (target->getType() == RENDER_TARGET_TYPE_BACKBUFFER) {
		m_currentRenderPassBuilder = ((Backbuffer*)m_currentRenderTarget)->getRenderPassBuilder();
	}
}

void VulkanBackend::setDepthParams(bool depth_test, bool depthWrite)
{
	m_depthStencilCreateInfo.depthTestEnable  = depth_test  ? VK_TRUE : VK_FALSE;
	m_depthStencilCreateInfo.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
}

void VulkanBackend::setDepthOp(VkCompareOp op)
{
	m_depthStencilCreateInfo.depthCompareOp = op;
}

void VulkanBackend::setDepthBoundsTest(bool enabled)
{
	m_depthStencilCreateInfo.depthTestEnable = enabled ? VK_TRUE : VK_FALSE;
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

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
	, m_graphicsQueue()
	, m_slangSession()
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
		.apiVersion = VK_API_VERSION_1_4
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

	findQueueFamilies();

	createLogicalDevice();

	volkLoadDevice(m_device);

	createVmaAllocator();

	createPipelineProcessCache();

	m_bindlessResources.init(this);

	initSlang();

	MGP_LOG("Vulkan Core Initialized!");
}

VulkanCore::~VulkanCore()
{
	MGP_LOG("0");

	deviceWaitIdle();
	
	MGP_LOG("1");

	m_slangSession.setNull(); // destroy
	
	MGP_LOG("2");

	m_pipelineCache.dispose();
	
	MGP_LOG("3");

	m_descriptorLayoutCache.dispose();
	
	MGP_LOG("4");

	m_bindlessResources.destroy();
	
	MGP_LOG("5");
	
	m_imGuiDescriptorPool.cleanUp();
	
	MGP_LOG("6");

	vkDestroyPipelineCache(m_device, m_pipelineProcessCache, nullptr);
	
	MGP_LOG("7");

	m_graphicsQueue.destroy();
	
	MGP_LOG("8");

	m_surface.destroy();
	
	MGP_LOG("9");

	vmaDestroyAllocator(m_vmaAllocator);
	
	MGP_LOG("10");

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
	if (!deviceCount)
		MGP_ERROR("Failed to find GPUs with Vulkan support!");

	VkPhysicalDeviceProperties2 properties		= { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	VkPhysicalDeviceFeatures2 features			= { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	vkGetPhysicalDeviceProperties2(devices[0], &properties);
	vkGetPhysicalDeviceFeatures2(devices[0], &features);

	m_physicalDevice = devices[0];
	m_physicalDeviceProperties = properties;
	m_physicalDeviceFeatures = features;

	// try to rank our physical devices and select one accordingly
	bool hasEssentials = false;
	uint32_t usability0 = vk_toolbox::assignPhysicalDeviceUsability(surface, m_physicalDevice, properties, features, &hasEssentials);

	// select the device of the highest usability
	int iSelected = 0;
	for (int i = 1; i < deviceCount; i++)
	{
		vkGetPhysicalDeviceProperties2(devices[i], &m_physicalDeviceProperties);
		vkGetPhysicalDeviceFeatures2(devices[i], &m_physicalDeviceFeatures);

		uint32_t usability1 = vk_toolbox::assignPhysicalDeviceUsability(surface, devices[i], properties, features, &hasEssentials);

		if (usability1 > usability0 && hasEssentials)
		{
			usability0 = usability1;

			m_physicalDevice = devices[i];
			m_physicalDeviceProperties = properties;
			m_physicalDeviceFeatures = features;

			iSelected = i;
		}
	}

	// get the actual number of samples we can take
	m_maxMsaaSamples = vk_toolbox::getMaxUsableSampleCount(m_physicalDeviceProperties);

	if (m_physicalDevice == VK_NULL_HANDLE)
		MGP_ERROR("Unable to find a suitable GPU!");

	MGP_LOG("Selected a suitable GPU: %d", iSelected);
}

void VulkanCore::createLogicalDevice()
{
	const std::vector<float> QUEUE_PRIORITIES = { 1.0f };

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	queueCreateInfos.push_back(m_graphicsQueue.getCreateInfo(QUEUE_PRIORITIES));

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

	m_physicalDeviceFeatures.features.robustBufferAccess = VK_FALSE;

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

	// create queues
	m_graphicsQueue.create(this, 0);

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
	VmaVulkanFunctions vulkanFunctions;
	vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
	vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
	vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
	vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
	vulkanFunctions.vkCreateImage = vkCreateImage;
	vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
	vulkanFunctions.vkDestroyImage = vkDestroyImage;
	vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vulkanFunctions.vkFreeMemory = vkFreeMemory;
	vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vulkanFunctions.vkMapMemory = vkMapMemory;
	vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
	vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;

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

void VulkanCore::findQueueFamilies()
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

	if (!queueFamilyCount)
		MGP_ERROR("Failed to find any queue families!");

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (int i = 0; i < queueFamilyCount; i++)
	{
		if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface.getHandle(), &presentSupport);

			if (presentSupport)
				m_graphicsQueue.setFamilyIndex(i);

			continue;
		}
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

void VulkanCore::nextFrame()
{
	m_currentFrameIndex = (m_currentFrameIndex + 1) % Queue::FRAMES_IN_FLIGHT;

	vkQueueWaitIdle(m_graphicsQueue.getHandle());
	m_graphicsQueue.getFrame(m_currentFrameIndex).pool.reset();
}

int VulkanCore::getCurrentFrameIndex() const
{
	return m_currentFrameIndex;
}

void VulkanCore::initImGui()
{
	m_imGuiDescriptorPool.init(this, 1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			1000 }
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

void VulkanCore::initSlang()
{
	Slang::ComPtr<slang::IGlobalSession> globalSession;
	slang::createGlobalSession(globalSession.writeRef());

	slang::SessionDesc sessionDesc = {};

	slang::TargetDesc targetDesc = {};
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = globalSession->findProfile("spirv_1_5");

	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	
	std::vector<slang::CompilerOptionEntry> options = 
	{
		{
			slang::CompilerOptionName::EmitSpirvDirectly,
			{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr }
		},
		{
			slang::CompilerOptionName::MatrixLayoutColumn,
			{ slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr }
		}
	};

	sessionDesc.compilerOptionEntries = options.data();
	sessionDesc.compilerOptionEntryCount = options.size();
	
	const char* searchPath = "../../res/shaders/src/";

	sessionDesc.searchPaths = &searchPath;
	sessionDesc.searchPathCount = 1;

	/*
	std::array<slang::PreprocessorMacroDesc, 2> preprocessorMacroDesc =
	{
		{ "BIAS_VALUE", "1138" },
		{ "OTHER_MACRO", "float" }
	};

	sessionDesc.preprocessorMacros = preprocessorMacroDesc.data();
	sessionDesc.preprocessorMacroCount = preprocessorMacroDesc.size();
	*/

	sessionDesc.preprocessorMacros = nullptr;
	sessionDesc.preprocessorMacroCount = 0;

	globalSession->createSession(sessionDesc, m_slangSession.writeRef());
}

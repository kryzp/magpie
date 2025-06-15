#include "graphics_core.h"

#include "third_party/imgui/imgui_impl_vulkan.h"

#include "platform/platform_core.h"

#include "core/common.h"

#include "vertex_format.h"
#include "swapchain.h"
#include "pipeline.h"
#include "toolbox.h"
#include "validation.h"
#include "gpu_buffer.h"
#include "sampler.h"
#include "image.h"
#include "image_view.h"
#include "shader.h"
#include "render_graph.h"
#include "descriptor.h"

using namespace mgp;

static std::vector<const char *> getInstanceExtensions(const PlatformCore *platform)
{
	uint32_t extCount = 0;
	const char *const *names = platform->vkGetInstanceExtensions(&extCount);

	if (!names)
		MGP_ERROR("Unable to get instance extension count.");

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

GraphicsCore::GraphicsCore(const Config &config, PlatformCore *platform)
	: m_platform(platform)
	, m_instance()
	, m_device()
	, m_physicalDevice()
	, m_physicalDeviceProperties()
	, m_physicalDeviceFeatures()
	, m_depthFormat()
	, m_maxMsaaSamples()
	, m_vmaAllocator()
	, m_currentFrameIndex()
	, m_pipelineProcessCache()
	, m_imGuiDescriptorPool(nullptr)
	, m_imGuiImageFormat()
	, m_swapchain(nullptr)
	, m_surface()
	, m_graphicsQueue()
	, m_slangGlobalSession()
	, m_slangSession()
	, m_inFlightCmd()
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

	auto extensions = getInstanceExtensions(m_platform);
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	
#if MGP_DEBUG
	vk_validation::trySetValidationLayers(createInfo);
#endif

#if MGP_MAC_SUPPORT
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	MGP_VK_CHECK(
		vkCreateInstance(&createInfo, nullptr, &m_instance),
		"Failed to create instance"
	);
	
	volkLoadInstance(m_instance);

#if MGP_DEBUG
	if (vk_validation::hasValidationLayers())
	{
		MGP_VK_CHECK(
			vk_validation::createDebugUtilsMessengerExt(m_instance, nullptr, &m_debugMessenger),
			"Failed to create debug messenger"
		);
	}
#endif

	m_surface.create(this, m_platform);

	enumeratePhysicalDevices(m_surface.getHandle());

	m_depthFormat = vk_toolbox::findDepthFormat(m_physicalDevice);

	findQueueFamilies();

	createLogicalDevice();

	volkLoadDevice(m_device);

	createVmaAllocator();

	createPipelineProcessCache();

	initSlang();

	m_swapchain = new Swapchain(this, m_platform);
	m_imGuiImageFormat = m_swapchain->getSwapchainImageFormat();

	MGP_LOG("Vulkan Core Initialized!");
}

GraphicsCore::~GraphicsCore()
{
	waitIdle();

	delete m_swapchain;
	
	delete m_imGuiDescriptorPool;

	vkDestroyPipelineCache(m_device, m_pipelineProcessCache, nullptr);
	
	m_graphicsQueue.destroy();
	
	m_surface.destroy();
	
	vmaDestroyAllocator(m_vmaAllocator);
	
	vkDestroyDevice(m_device, nullptr);

	MGP_LOG("Vulkan Core Destroyed!");
}

CommandBuffer *GraphicsCore::beginPresent()
{
	auto &currentFrame = m_graphicsQueue.getFrame(m_currentFrameIndex);

	waitForFence(currentFrame.inFlightFence);
	resetFence(currentFrame.inFlightFence);

	m_swapchain->acquireNextImage();

	m_inFlightCmd = CommandBuffer(currentFrame.pool.getFreeBuffer());
	m_inFlightCmd.begin();

	return &m_inFlightCmd;
}

void GraphicsCore::present()
{
	m_inFlightCmd.transitionLayout(m_swapchain->getCurrentSwapchainImage(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	m_inFlightCmd.end();

	VkFence fence = m_graphicsQueue.getFrame(m_currentFrameIndex).inFlightFence;

	VkSemaphoreSubmitInfo imageAvailableSemaphore = m_swapchain->getImageAvailableSemaphoreSubmitInfo();
	VkSemaphoreSubmitInfo renderFinishedSemaphore = m_swapchain->getRenderFinishedSemaphoreSubmitInfo();

	VkCommandBufferSubmitInfo bufferInfo = m_inFlightCmd.getSubmitInfo();

	VkSubmitInfo2 submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.flags = 0;

	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &bufferInfo;

	submitInfo.signalSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &renderFinishedSemaphore;

	submitInfo.waitSemaphoreInfoCount = 1;
	submitInfo.pWaitSemaphoreInfos = &imageAvailableSemaphore;
	
	MGP_VK_CHECK(
		vkQueueSubmit2(m_graphicsQueue.getHandle(), 1, &submitInfo, fence),
		"Failed to submit in-flight draw command to buffer"
	);

	VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo = m_swapchain->getRenderFinishedSemaphoreSubmitInfo();
	unsigned imageIndex = m_swapchain->getCurrentSwapchainImageIndex();

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitSemaphoreSubmitInfo.semaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain->getHandle();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VkResult result = vkQueuePresentKHR(m_graphicsQueue.getHandle(), &presentInfo);

	// rebuild swapchain if failure
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		m_swapchain->rebuildSwapchain();
	}

	// just crash if the error still not a success but not known
	else if (result != VK_SUCCESS)
	{
		MGP_ERROR("Failed to present swap chain image: %d", result);
	}

	m_currentFrameIndex = (m_currentFrameIndex + 1) % gfx_constants::FRAMES_IN_FLIGHT;

	vkQueueWaitIdle(m_graphicsQueue.getHandle());
	m_graphicsQueue.getFrame(m_currentFrameIndex).pool.reset();
}

CommandBuffer *GraphicsCore::beginInstantSubmit()
{
	auto &currentFrame = m_graphicsQueue.getFrame(m_currentFrameIndex);

	waitForFence(currentFrame.instantSubmitFence);
	resetFence(currentFrame.instantSubmitFence);

	CommandBuffer *cmd = new CommandBuffer(currentFrame.pool.getFreeBuffer());
	cmd->begin();

	return cmd;
}

void GraphicsCore::submit(CommandBuffer *cmd)
{
	cmd->end();

	VkCommandBufferSubmitInfo bufferInfo = ((CommandBuffer *)cmd)->getSubmitInfo();

	delete cmd;

	VkFence fence = m_graphicsQueue.getFrame(m_currentFrameIndex).instantSubmitFence;

	VkSubmitInfo2 submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.flags = 0;

	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &bufferInfo;

	submitInfo.signalSemaphoreInfoCount = 0;
	submitInfo.waitSemaphoreInfoCount = 0;

	MGP_VK_CHECK(
		vkQueueSubmit2(m_graphicsQueue.getHandle(), 1, &submitInfo, fence),
		"Failed to submit instant draw command to buffer"
	);
}

void GraphicsCore::enumeratePhysicalDevices(VkSurfaceKHR surface)
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

void GraphicsCore::createLogicalDevice()
{
	const std::vector<float> QUEUE_PRIORITIES = { 1.0f };

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	queueCreateInfos.push_back(m_graphicsQueue.getCreateInfo(QUEUE_PRIORITIES));

	m_physicalDeviceFeatures.features.robustBufferAccess = VK_FALSE;

	VkPhysicalDeviceVulkan11Features vulkan11Features = {};
	vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	vulkan11Features.shaderDrawParameters = VK_TRUE;
	vulkan11Features.pNext = nullptr;

	VkPhysicalDeviceVulkan12Features vulkan12Features = {};
	vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan12Features.runtimeDescriptorArray = VK_TRUE;
	vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
	vulkan12Features.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	vulkan12Features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	vulkan12Features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	vulkan12Features.bufferDeviceAddress = VK_TRUE;
	vulkan12Features.pNext = &vulkan11Features;

	VkPhysicalDeviceVulkan13Features vulkan13Features = {};
	vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan13Features.dynamicRendering = VK_TRUE;
	vulkan13Features.synchronization2 = VK_TRUE;
	vulkan13Features.pNext = &vulkan12Features;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.enabledExtensionCount = MGP_ARRAY_LENGTH(vk_toolbox::DEVICE_EXTENSIONS);
	createInfo.ppEnabledExtensionNames = vk_toolbox::DEVICE_EXTENSIONS;
	createInfo.pEnabledFeatures = &m_physicalDeviceFeatures.features;
	createInfo.pNext = &vulkan13Features;

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

void GraphicsCore::createPipelineProcessCache()
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

void GraphicsCore::findQueueFamilies()
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

void GraphicsCore::createVmaAllocator()
{
	VmaVulkanFunctions vulkanFunctions;
	vulkanFunctions.vkAllocateMemory						= vkAllocateMemory;
	vulkanFunctions.vkBindBufferMemory						= vkBindBufferMemory;
	vulkanFunctions.vkBindImageMemory						= vkBindImageMemory;
	vulkanFunctions.vkCreateBuffer							= vkCreateBuffer;
	vulkanFunctions.vkCreateImage							= vkCreateImage;
	vulkanFunctions.vkDestroyBuffer							= vkDestroyBuffer;
	vulkanFunctions.vkDestroyImage							= vkDestroyImage;
	vulkanFunctions.vkFlushMappedMemoryRanges				= vkFlushMappedMemoryRanges;
	vulkanFunctions.vkFreeMemory							= vkFreeMemory;
	vulkanFunctions.vkGetBufferMemoryRequirements			= vkGetBufferMemoryRequirements;
	vulkanFunctions.vkGetImageMemoryRequirements			= vkGetImageMemoryRequirements;
	vulkanFunctions.vkGetPhysicalDeviceMemoryProperties		= vkGetPhysicalDeviceMemoryProperties;
	vulkanFunctions.vkGetPhysicalDeviceProperties			= vkGetPhysicalDeviceProperties;
	vulkanFunctions.vkInvalidateMappedMemoryRanges			= vkInvalidateMappedMemoryRanges;
	vulkanFunctions.vkMapMemory								= vkMapMemory;
	vulkanFunctions.vkUnmapMemory							= vkUnmapMemory;
	vulkanFunctions.vkCmdCopyBuffer							= vkCmdCopyBuffer;

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

void GraphicsCore::initImGui()
{
	m_imGuiDescriptorPool = new DescriptorPoolStatic(this, 1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 						1.0f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 				1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 				1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,			1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 				1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 				1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 				1.0f }
	});

	ImGui_ImplVulkan_InitInfo info = {};
	info.Instance = m_instance;
	info.PhysicalDevice = m_physicalDevice;
	info.Device = m_device;
	info.QueueFamily = m_graphicsQueue.getFamilyIndex();
	info.Queue = m_graphicsQueue.getHandle();
	info.PipelineCache = m_pipelineProcessCache;
	info.DescriptorPool = m_imGuiDescriptorPool->getPool();
	info.Allocator = nullptr;
	info.MinImageCount = gfx_constants::FRAMES_IN_FLIGHT;
	info.ImageCount = gfx_constants::FRAMES_IN_FLIGHT;
	info.CheckVkResultFn = nullptr;
	info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	info.UseDynamicRendering = true;
	info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_imGuiImageFormat;

	ImGui_ImplVulkan_Init(&info);
	ImGui_ImplVulkan_CreateFontsTexture();
}

void GraphicsCore::initSlang()
{
	slang::createGlobalSession(m_slangGlobalSession.writeRef());

	slang::SessionDesc sessionDesc = {};

	slang::TargetDesc targetDesc = {};
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = m_slangGlobalSession->findProfile("spirv_1_5");

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

	m_slangGlobalSession->createSession(sessionDesc, m_slangSession.writeRef());
}

void GraphicsCore::waitIdle()
{
	vkDeviceWaitIdle(m_device);
}

void GraphicsCore::waitForFence(const VkFence &fence) const
{
	vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void GraphicsCore::resetFence(const VkFence &fence) const
{
	vkResetFences(m_device, 1, &fence);
}

VkFormat GraphicsCore::getDepthFormat()
{
	return m_depthFormat;
}

Swapchain *GraphicsCore::getSwapchain()
{
	return m_swapchain;
}

Image *GraphicsCore::createImage(
	uint32_t width, uint32_t height, uint32_t depth,
	VkFormat format,
	VkImageViewType type,
	VkImageTiling tiling,
	uint32_t mipmaps,
	VkSampleCountFlagBits samples,
	bool transient,
	bool storage
)
{
	Image *image = new Image();

	image->allocate(
		this,
		width, height, depth,
		format,
		type,
		tiling,
		mipmaps,
		samples,
		transient,
		storage
	);

	return image;
}

ImageView *GraphicsCore::createImageView(
	Image *image,
	int layerCount,
	int layer,
	int baseMipLevel
)
{
	return new ImageView(this, (Image *)image, layerCount, layer, baseMipLevel);
}

Sampler *GraphicsCore::createSampler(const SamplerStyle &style)
{
	return new Sampler(this, style);
}

ShaderStage *GraphicsCore::createShaderStage(
	VkShaderStageFlagBits type,
	const std::string &path
)
{
	return new ShaderStage(this, type, path);
}

Shader *GraphicsCore::createShader(
	uint64_t pushConstantsSize,
	const std::vector<DescriptorLayout *> &layouts,
	const std::vector<ShaderStage *> &stages
)
{
	return new Shader(this, pushConstantsSize, layouts, stages);
}

GPUBuffer *GraphicsCore::createGPUBuffer(
	VkBufferUsageFlags usage,
	VmaAllocationCreateFlagBits flags,
	uint64_t size
)
{
	return new GPUBuffer(this, usage, flags, size);
}

DescriptorLayout *GraphicsCore::createDescriptorLayout(
	VkShaderStageFlags shaderStages,
	const std::vector<DescriptorLayoutBinding> &bindings,
	VkDescriptorSetLayoutCreateFlags flags
)
{
	std::vector<VkDescriptorSetLayoutBinding> vkBindings;
	std::vector<VkDescriptorBindingFlags> vkBindingFlags;

	for (auto &b : bindings)
	{
		VkDescriptorSetLayoutBinding vkBinding = {};
		vkBinding.binding = b.index;
		vkBinding.descriptorType = b.type;
		vkBinding.descriptorCount = b.count;
		vkBinding.stageFlags = shaderStages;
		vkBinding.pImmutableSamplers = nullptr;

		vkBindings.push_back(vkBinding);
		vkBindingFlags.push_back(b.bindingFlags);
	}
	
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.flags = flags;
	layoutCreateInfo.bindingCount = vkBindings.size();
	layoutCreateInfo.pBindings = vkBindings.data();

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlags.bindingCount = vkBindingFlags.size();
	bindingFlags.pBindingFlags = vkBindingFlags.data();

	if (!vkBindingFlags.empty())
		layoutCreateInfo.pNext = &bindingFlags;
	
	VkDescriptorSetLayout layout = {};

	MGP_VK_CHECK(
		vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &layout),
		"Failed to create descriptor set layout"
	);

	return new DescriptorLayout(this, layout);
}

DescriptorPoolStatic *GraphicsCore::createStaticDescriptorPool(uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes)
{
	return new DescriptorPoolStatic(this, maxSets, flags, sizes);
}

DescriptorPool *GraphicsCore::createDescriptorPool(uint32_t initialSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes)
{
	return new DescriptorPool(this, initialSets, flags, sizes);
}

VkPipelineLayout GraphicsCore::createPipelineLayout(const Shader *shader)
{
	VkShaderStageFlags shaderStage = shader->getStages()[0]->getType() == VK_SHADER_STAGE_COMPUTE_BIT ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS;

	auto &layouts = shader->getLayouts();

	std::vector<VkDescriptorSetLayout> vkLayouts(layouts.size());

	for (int i = 0; i < layouts.size(); i++) {
		vkLayouts[i] = ((DescriptorLayout *)layouts[i])->getLayout();
	}

	VkPushConstantRange pushConstants = {};
	pushConstants.offset = 0;
	pushConstants.size = shader->getPushConstantSize();
	pushConstants.stageFlags = shaderStage;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = vkLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = vkLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (pushConstants.size > 0)
	{
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	MGP_VK_CHECK(
		vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &layout),
		"Failed to create pipeline layout"
	);

	return layout;
}

VkPipeline GraphicsCore::createGraphicsPipeline(VkPipelineLayout layout, const GraphicsPipelineDef &definition, const RenderInfo &renderInfo)
{
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
	
	std::vector<VkVertexInputBindingDescription> vkBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> vkAttributeDescriptions;

	if (definition.getVertexFormat())
	{
		cauto &bindingDescriptions = definition.getVertexFormat()->getBindings();
		cauto &attributeDescriptions = definition.getVertexFormat()->getAttributes();

		for (auto &b : bindingDescriptions)
		{
			VkVertexInputBindingDescription binding;
			binding.binding = b.binding;
			binding.inputRate = b.inputRate;
			binding.stride = b.stride;

			vkBindingDescriptions.push_back(binding);
		}

		for (auto &a : attributeDescriptions)
		{
			VkVertexInputAttributeDescription attribute;
			attribute.binding = a.binding;
			attribute.format = a.format;
			attribute.location = a.location;
			attribute.offset = a.offset;

			vkAttributeDescriptions.push_back(attribute);
		}

		vertexInputStateCreateInfo.vertexBindingDescriptionCount = vkBindingDescriptions.size();
		vertexInputStateCreateInfo.pVertexBindingDescriptions = vkBindingDescriptions.data();
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vkAttributeDescriptions.size();
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = vkAttributeDescriptions.data();
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = nullptr; // using dynamic viewport
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = nullptr; // using dynamic scissor

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = definition.getCullMode();
	rasterizationStateCreateInfo.frontFace = definition.getFrontFace();
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = definition.isSampleShadingEnabled() ? VK_TRUE : VK_FALSE;
	multisampleStateCreateInfo.minSampleShading = definition.getMinSampleShading();
	multisampleStateCreateInfo.rasterizationSamples = renderInfo.getMSAA();
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	std::vector<VkPipelineColorBlendAttachmentState> blendStates(renderInfo.getColourAttachments().size());

	for (int i = 0; i < renderInfo.getColourAttachments().size(); i++) {
		blendStates[i] = definition.getColourBlendState();
	}

	VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo = {};
	colourBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendStateCreateInfo.logicOpEnable = definition.isBlendStateLogicOpEnabled() ? VK_TRUE : VK_FALSE;
	colourBlendStateCreateInfo.logicOp = definition.getBlendStateLogicOp();
	colourBlendStateCreateInfo.attachmentCount = blendStates.size();
	colourBlendStateCreateInfo.pAttachments = blendStates.data();
	colourBlendStateCreateInfo.blendConstants[0] = definition.getBlendConstant(0);
	colourBlendStateCreateInfo.blendConstants[1] = definition.getBlendConstant(1);
	colourBlendStateCreateInfo.blendConstants[2] = definition.getBlendConstant(2);
	colourBlendStateCreateInfo.blendConstants[3] = definition.getBlendConstant(3);

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = definition.getDepthStencilState();

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = MGP_ARRAY_LENGTH(PIPELINE_DYNAMIC_STATES);
	dynamicStateCreateInfo.pDynamicStates = PIPELINE_DYNAMIC_STATES;

	cauto &colourFormats = renderInfo.getColourAttachmentFormats();
	VkFormat depthStencilFormat = m_depthFormat;

	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipelineRenderingCreateInfo.colorAttachmentCount = colourFormats.size();
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colourFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = depthStencilFormat;
	pipelineRenderingCreateInfo.stencilAttachmentFormat = depthStencilFormat;

	cauto &shaderStages = definition.getShader()->getStages();
	std::vector<VkPipelineShaderStageCreateInfo> vkShaderStages(shaderStages.size());

	for (int i = 0; i < shaderStages.size(); i++) {
		vkShaderStages[i] = shaderStages[i]->getShaderStageCreateInfo();
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = vkShaderStages.size();
	graphicsPipelineCreateInfo.pStages = vkShaderStages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colourBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = layout;
	graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

	VkPipeline pipeline = VK_NULL_HANDLE;

	MGP_VK_CHECK(
		vkCreateGraphicsPipelines(m_device, m_pipelineProcessCache, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline),
		"Failed to create new graphics pipeline"
	);

	return pipeline;
}

VkPipeline GraphicsCore::createComputePipeline(VkPipelineLayout layout, const ComputePipelineDef &definition)
{
	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.stage = definition.getShader()->getStages()[0]->getShaderStageCreateInfo();

	VkPipeline pipeline = VK_NULL_HANDLE;

	MGP_VK_CHECK(
		vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline),
		"Failed to create new compute pipeline"
	);

	return pipeline;
}

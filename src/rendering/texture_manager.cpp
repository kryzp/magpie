#include "texture_manager.h"

#include "core/common.h"

#include "vulkan/core.h"
#include "vulkan/bitmap.h"
#include "vulkan/image.h"
#include "vulkan/sampler.h"
#include "vulkan/gpu_buffer.h"
#include "vulkan/sync.h"

using namespace mgp;

TextureManager::TextureManager()
	: m_loadedImageCache()
	, m_linearSampler(nullptr)
	, m_nearestSampler(nullptr)
{
}

TextureManager::~TextureManager()
{
}

void TextureManager::init(VulkanCore *core)
{
	m_core = core;

	loadTextures();
}

void TextureManager::destroy()
{
	for (cauto &[name, image] : m_loadedImageCache)
		delete image;

	delete m_linearSampler;
	delete m_nearestSampler;
}

Image *TextureManager::getTexture(const std::string &name)
{
	if (m_loadedImageCache.contains(name))
		return m_loadedImageCache.at(name);

	return nullptr;
}

Image *TextureManager::loadTexture(const std::string &name, const std::string &path)
{
	if (m_loadedImageCache.contains(name))
		return m_loadedImageCache.at(name);

	Bitmap bitmap(path);

	Image *image = new Image();
	image->allocate(
		m_core,
		bitmap.getWidth(), bitmap.getHeight(), 1,
		bitmap.getFormat() == Bitmap::FORMAT_RGBA8 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		4,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	GPUBuffer stagingBuffer(
		m_core,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		bitmap.getMemorySize()
	);

	stagingBuffer.write(bitmap.getData(), bitmap.getMemorySize(), 0);

	InstantSubmitSync instantSubmit(m_core);
	CommandBuffer &cmd = instantSubmit.begin();
	{
		cmd.transitionLayout(*image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		cmd.copyBufferToImage(stagingBuffer, *image);
		cmd.generateMipmaps(*image);
	}
	instantSubmit.submit();

	m_loadedImageCache.insert({ name, image });

	// wait when staging buffer needs to delete
	// todo: yes, this is terrible and there really should just
	//       be a single staging buffer used for the whole program
	m_core->deviceWaitIdle();

	return image;
}

void TextureManager::loadTextures()
{
	m_linearSampler = new Sampler(m_core, Sampler::Style(VK_FILTER_LINEAR));
	m_nearestSampler = new Sampler(m_core, Sampler::Style(VK_FILTER_NEAREST));

	loadTexture("fallback_white",	"../../res/textures/standard/white.png");
	loadTexture("fallback_black",	"../../res/textures/standard/black.png");
	loadTexture("fallback_normals",	"../../res/textures/standard/normal_fallback.png");

	loadTexture("environmentHDR",	"../../res/textures/rogland_clear_night_greg_zaal.hdr");

	loadTexture("stone",			"../../res/textures/smooth_stone.png");
	loadTexture("wood",				"../../res/textures/wood.jpg");
}

Image *TextureManager::getFallbackDiffuse()
{
	return getTexture("fallback_white");
}

Image *TextureManager::getFallbackAmbient()
{
	return getTexture("fallback_white");
}

Image *TextureManager::getFallbackRoughnessMetallic()
{
	return getTexture("fallback_black");
}

Image *TextureManager::getFallbackNormals()
{
	return getTexture("fallback_normals");
}

Image *TextureManager::getFallbackEmissive()
{
	return getTexture("fallback_black");
}

Sampler *TextureManager::getLinearSampler()
{
	return m_linearSampler;
}

Sampler *TextureManager::getNearestSampler()
{
	return m_nearestSampler;
}

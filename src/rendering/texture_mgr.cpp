#include "texture_mgr.h"

#include "vulkan/texture.h"
#include "vulkan/texture_sampler.h"
#include "vulkan/core.h"
#include "vulkan/descriptor_builder.h"

llt::TextureMgr *llt::g_textureManager = nullptr;

using namespace llt;

TextureMgr::TextureMgr()
	: m_textureCache()
	, m_samplerCache()
{
}

TextureMgr::~TextureMgr()
{
	for (auto &[name, texture] : m_textureCache) {
		delete texture;
	}

	m_textureCache.clear();

	for (auto &[name, sampler] : m_samplerCache) {
		delete sampler;
	}

	m_samplerCache.clear();
}

void TextureMgr::loadDefaultTexturesAndSamplers()
{
	createSampler("linear", TextureSampler::Style(VK_FILTER_LINEAR));
	createSampler("nearest", TextureSampler::Style(VK_FILTER_NEAREST));

	load("fallback_white",		"../../res/textures/standard/white.png");
	load("fallback_black",		"../../res/textures/standard/black.png");
	load("fallback_normals",	"../../res/textures/standard/normal_fallback.png");

	load("environmentHDR",		"../../res/textures/spruit_sunrise_greg_zaal.hdr");

	load("stone",				"../../res/textures/smooth_stone.png");
	load("wood",				"../../res/textures/wood.jpg");
}

TextureSampler *TextureMgr::getSampler(const String &name)
{
	return m_samplerCache.getOrDefault(name, nullptr);
}

Texture *TextureMgr::getTexture(const String &name)
{
	return m_textureCache.getOrDefault(name, nullptr);
}

Texture *TextureMgr::load(const String &name, const String &path)
{
	Image image(path.cstr());
	return createFromImage(name, image);
}

Texture *TextureMgr::createFromImage(const String &name, const Image &image)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture *texture = new Texture();

	texture->fromImage(image, VK_IMAGE_VIEW_TYPE_2D, 4, VK_SAMPLE_COUNT_1_BIT);
	texture->setMipLevels(1);
	texture->createInternalResources();
	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	g_gpuBufferManager->textureStagingBuffer->writeDataToMe(image.getData(), image.getSize(), 0);
	g_gpuBufferManager->textureStagingBuffer->writeToTextureSingle(texture, image.getSize());

	CommandBuffer cmd = vkutil::beginSingleTimeCommands(g_vkCore->getGraphicsCommandPool());
	texture->generateMipmaps(cmd);
	vkutil::endSingleTimeGraphicsCommands(cmd);

	m_textureCache.insert(name, texture);
	return texture;
}

Texture *TextureMgr::createFromData(const String &name, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, const byte *data, uint64_t size)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture *texture = new Texture();

	texture->setSize(width, height);
	texture->setProperties(format, tiling, VK_IMAGE_VIEW_TYPE_2D);
	texture->createInternalResources();
	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	if (data)
	{
		g_gpuBufferManager->textureStagingBuffer->writeDataToMe(data, size, 0);
		g_gpuBufferManager->textureStagingBuffer->writeToTextureSingle(texture, size);

		texture->setMipLevels(4);

		CommandBuffer cmd = vkutil::beginSingleTimeCommands(g_vkCore->getGraphicsCommandPool());
		texture->generateMipmaps(cmd);
		vkutil::endSingleTimeGraphicsCommands(cmd);
	}
	else
	{
		texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	m_textureCache.insert(name, texture);
	return texture;
}

Texture *TextureMgr::createAttachment(const String &name, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture *texture = new Texture();

	texture->setSize(width, height);
	texture->setProperties(format, tiling, VK_IMAGE_VIEW_TYPE_2D);
	texture->createInternalResources();
	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_textureCache.insert(name, texture);
	return texture;
}

Texture *TextureMgr::createCubemap(const String &name, uint32_t size, VkFormat format, int mipLevels)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture *texture = new Texture();

	texture->setSize(size, size);
	texture->setProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_CUBE);
	texture->setMipLevels(mipLevels);
	texture->createInternalResources();
	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	m_textureCache.insert(name, texture);
	return texture;
}

Texture *TextureMgr::createCubemap(const String &name, const Image &right, const Image &left, const Image &top, const Image &bottom, const Image &front, const Image &back, int mipLevels)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture *texture = new Texture();

	texture->setSize(right.getWidth(), right.getWidth());
	texture->setProperties(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_CUBE);
	texture->setMipLevels(mipLevels);
	texture->createInternalResources();
	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	const Image *sides[] = { &right, &left, &top, &bottom, &front, &back };

	for (int i = 0; i < 6; i++)
	{
		g_gpuBufferManager->textureStagingBuffer->writeDataToMe(sides[i]->getData(), sides[i]->getSize(), sides[i]->getSize() * i);
	}

	CommandBuffer cmd = vkutil::beginSingleTimeCommands(g_vkCore->getTransferCommandPool());

	for (int i = 0; i < 6; i++)
	{
		g_gpuBufferManager->textureStagingBuffer->writeToTexture(cmd, texture, sides[i]->getSize(), sides[i]->getSize() * i, i);
	}

	texture->generateMipmaps(cmd);

	vkutil::endSingleTimeGraphicsCommands(cmd);

	m_textureCache.insert(name, texture);
	return texture;
}

TextureSampler *TextureMgr::createSampler(const String &name, const TextureSampler::Style &style)
{
	if (m_samplerCache.contains(name)) {
		return m_samplerCache.get(name);
	}

	TextureSampler *sampler = new TextureSampler(style);

	m_samplerCache.insert(name, sampler);
	return sampler;
}

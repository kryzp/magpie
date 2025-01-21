#include "texture_mgr.h"
#include "texture.h"
#include "texture_sampler.h"
#include "backend.h"
#include "descriptor_builder.h"

llt::TextureMgr* llt::g_textureManager = nullptr;

using namespace llt;

TextureMgr::TextureMgr()
	: m_textureCache()
	, m_samplerCache()
{
}

TextureMgr::~TextureMgr()
{
	for (auto& [name, texture] : m_textureCache) {
		delete texture;
	}

	m_textureCache.clear();

	for (auto& [name, sampler] : m_samplerCache) {
		delete sampler;
	}

	m_samplerCache.clear();
}

TextureSampler* TextureMgr::getSampler(const String& name)
{
	if (m_samplerCache.contains(name)) {
		return m_samplerCache[name];
	}

	return nullptr;
}

Texture* TextureMgr::getTexture(const String& name)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	return nullptr;
}

Texture* TextureMgr::createFromImage(const String& name, const Image& image)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture* texture = new Texture();

	texture->fromImage(image, VK_IMAGE_VIEW_TYPE_2D, 4, VK_SAMPLE_COUNT_1_BIT);

	texture->setMipLevels(1);

	texture->createInternalResources();

	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	g_gpuBufferManager->textureStagingBuffer->writeDataToMe(image.getData(), image.getSize(), 0);
	g_gpuBufferManager->textureStagingBuffer->writeToTexture(texture, image.getSize());

	m_textureCache.insert(name, texture);
	return texture;
}

Texture* TextureMgr::createFromData(const String& name, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, const byte* data, uint64_t size)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture* texture = new Texture();

	texture->setSize(width, height);
	texture->setProperties(format, tiling, VK_IMAGE_VIEW_TYPE_2D);

	texture->createInternalResources();
    
	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	if (data)
	{
		texture->setMipLevels(4);

		g_gpuBufferManager->textureStagingBuffer->writeDataToMe(data, size, 0);
		g_gpuBufferManager->textureStagingBuffer->writeToTexture(texture, size);
	}
	else
	{
		texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	m_textureCache.insert(name, texture);
	return texture;
}

Texture* TextureMgr::createAttachment(const String& name, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture* texture = new Texture();

	texture->setSize(width, height);
	texture->setProperties(format, tiling, VK_IMAGE_VIEW_TYPE_2D);

	texture->createInternalResources();

	// todo: WHAT???
	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_textureCache.insert(name, texture);
	return texture;
}

Texture* TextureMgr::createCubeMap(const String& name, VkFormat format, const Image& right, const Image& left, const Image& top, const Image& bottom, const Image& front, const Image& back)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture* texture = new Texture();

	texture->setSize(right.getWidth(), right.getHeight());
	texture->setProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_CUBE);

	texture->setMipLevels(4);

	texture->createInternalResources();

	texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	const Image* sides[] = { &right, &left, &top, &bottom, &front, &back };

	for (int i = 0; i < 6; i++)
	{
		g_gpuBufferManager->textureStagingBuffer->writeDataToMe(sides[i]->getData(), sides[i]->getSize(), sides[i]->getSize() * i);
	}

	for (int i = 0; i < 6; i++)
	{
		g_gpuBufferManager->textureStagingBuffer->writeToTexture(texture, sides[i]->getSize(), sides[i]->getSize() * i, i);
	}

	m_textureCache.insert(name, texture);
	return texture;
}

TextureSampler* TextureMgr::createSampler(const String& name, const TextureSampler::Style& style)
{
	if (m_samplerCache.contains(name)) {
		return m_samplerCache.get(name);
	}

	TextureSampler* sampler = new TextureSampler(style);

	m_samplerCache.insert(name, sampler);
	return sampler;
}

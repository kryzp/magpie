
#include "texture_mgr.h"
#include "texture.h"
#include "texture_sampler.h"
#include "backend.h"

llt::TextureMgr* llt::g_textureManager = nullptr;

using namespace llt;

TextureMgr::TextureMgr()
	: m_textureCache()
	, m_samplerCache()
{
}

TextureMgr::~TextureMgr()
{
	for (auto& [id, texture] : m_textureCache) {
		delete texture;
	}

	m_textureCache.clear();

	for (auto& [id, sampler] : m_samplerCache) {
		delete sampler;
	}

	m_samplerCache.clear();
}

Texture* TextureMgr::getTexture(const String& name)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache[name];
	}

	return nullptr;
}

TextureSampler* TextureMgr::getSampler(const String& name, const TextureSampler::Style& style)
{
	if (m_samplerCache.contains(name)) {
		return m_samplerCache[name];
	}

	TextureSampler* sampler = new TextureSampler(style);
	m_samplerCache.insert(Pair(name, sampler));

	return sampler;
}

Texture* TextureMgr::create(const Image& image)
{
	Texture* texture = new Texture();

	// create the texture from the image with a sampling count of 1 and 4 mipmaps
	texture->fromImage(image, VK_IMAGE_VIEW_TYPE_2D, 4, VK_SAMPLE_COUNT_1_BIT);

	// all images have levels of mipmaps
	texture->setMipmapped(true);

	// create the internal resources for the texture
	texture->createInternalResources();

	// transition into the transfer destination layout for the staging buffer
	texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// move the image data onto the staging buffer, and then copy the data on the staging buffer onto the texture
	Buffer* stage = g_bufferManager->createStagingBuffer(image.getSize());
	stage->readDataFromMemory(image.getData(), image.getSize(), 0);
	stage->writeToTexture(texture, image.getSize());
	delete stage;

	return texture;
}

Texture* TextureMgr::create(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, const byte* data, uint64_t size)
{
	Texture* texture = new Texture();

	texture->initSize(width, height);
	texture->initMetadata(format, tiling, VK_IMAGE_VIEW_TYPE_2D);
	texture->initMipLevels(4);
	texture->createInternalResources();
    texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// check if we're actually creating a texture from data or if we're just trying to create an empty texture for now (data = nullptr)
	if (data)
	{
		// this means we need to be mipmapped
		texture->setMipmapped(true);

		// transfer data into the texture via staging buffer
		Buffer* stage = g_bufferManager->createStagingBuffer(size);
		stage->readDataFromMemory(data, size, 0);
		stage->writeToTexture(texture, size);
		delete stage;
	}
	else
	{
		// simply just transition into the shader reading layout
		texture->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	return texture;
}

Texture* TextureMgr::createAttachment(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling)
{
	Texture* texture = new Texture();

	texture->initSize(width, height);
	texture->initMetadata(format, tiling, VK_IMAGE_VIEW_TYPE_2D);

	texture->createInternalResources();

	texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	texture->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return texture;
}

Texture* TextureMgr::createCubeMap(VkFormat format, const Image& right, const Image& left, const Image& top, const Image& bottom, const Image& front, const Image& back)
{
	Texture* texture = new Texture();

	texture->initSize(right.getWidth(), right.getHeight());
	texture->initMetadata(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_CUBE);
	texture->initMipLevels(4);

	texture->setMipmapped(true);

	texture->createInternalResources();

	texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// create a staging buffer large enough for all 6 textures
	Buffer* stage = g_bufferManager->createStagingBuffer(right.getSize() * 6);

	const Image* sides[] = { &right, &left, &top, &bottom, &front, &back };

	for (int i = 0; i < 6; i++) {
		// transfer the data to the texture at some offset based on the index of the face
		stage->readDataFromMemory(sides[i]->getData(), sides[i]->getSize(), sides[i]->getSize() * i);
		stage->writeToTexture(texture, sides[i]->getSize(), sides[i]->getSize() * i, i);
	}

	delete stage;

	return texture;
}

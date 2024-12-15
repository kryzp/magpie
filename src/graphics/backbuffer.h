#ifndef VK_BACKBUFFER_H_
#define VK_BACKBUFFER_H_

#include <glm/vec4.hpp>

#include <vulkan/vulkan.h>

#include "generic_render_target.h"
#include "texture.h"
#include "util.h"
#include "render_info_builder.h"

namespace llt
{
	class VulkanBackend;

	class Backbuffer : public GenericRenderTarget
	{
	public:
		Backbuffer();
		~Backbuffer() override;

		void create();
		void createSurface();

		void createSwapChain();
		void createSwapChainImageViews();

		void cleanUp() override;
		void cleanUpSwapChain();
		void cleanUpTextures();

		void beginGraphics(VkCommandBuffer cmdBuffer) override;
		void endGraphics(VkCommandBuffer cmdBuffer) override;

		void setClearColour(int idx, const Colour& colour) override;

		void setDepthStencilClear(float depth, uint32_t stencil) override;

		void acquireNextImage();

		void swapBuffers();

		Texture* getAttachment(int idx) override;
		Texture* getDepthAttachment() override;

		void onWindowResize(int width, int height);

		VkSurfaceKHR getSurface() const;

		VkSampleCountFlagBits getMSAA() const override;

		int getCurrentTextureIdx() const;

		const VkSemaphore& getRenderFinishedSemaphore() const;
		const VkSemaphore& getImageAvailableSemaphore() const;

	private:
		void createColourResources(); // coloured visual component
		void createDepthResources(); // depth component

		void createSwapChainSyncObjects();

		void rebuildSwapChain();

		Array<VkSemaphore, mgc::FRAMES_IN_FLIGHT> m_renderFinishedSemaphores;
		Array<VkSemaphore, mgc::FRAMES_IN_FLIGHT> m_imageAvailableSemaphores;

		VkSwapchainKHR m_swapChain;
		
		Vector<VkImage> m_swapChainImages;
		Vector<VkImageView> m_swapChainImageViews;

		VkSurfaceKHR m_surface;
		uint32_t m_currSwapChainImageIdx;

		Texture m_colour;
		Texture m_depth;
	};
}

#endif // VK_BACKBUFFER_H_

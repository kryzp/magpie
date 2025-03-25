#ifndef VK_SWAPCHAIN_H_
#define VK_SWAPCHAIN_H_

#include <glm/vec4.hpp>

#include "third_party/volk.h"

#include "generic_render_target.h"
#include "texture.h"
#include "util.h"
#include "render_info.h"

namespace llt
{
	class VulkanCore;

	// swapchain wrapper
	class Swapchain : public GenericRenderTarget
	{
	public:
		Swapchain();
		~Swapchain() override;

		void create();
		void createSurface();

		void createSwapChain();
		void createSwapChainImageViews();

		void cleanUp() override;
		void cleanUpSwapChain();
		void cleanUpTextures();

		void beginRendering(CommandBuffer &cmd) override;
		void endRendering(CommandBuffer &cmd) override;

		void setClearColour(int idx, const Colour &colour) override;

		void setDepthStencilClear(float depth, uint32_t stencil) override;

		void acquireNextImage();

		void swapBuffers();

		Texture *getAttachment(int idx) override;
		Texture *getDepthAttachment() override;

		VkImage getCurrentSwapchainImage() const;
		VkImageView getCurrentSwapchainImageView() const;

		void onWindowResize(int width, int height);

		VkSurfaceKHR getSurface() const;

		const VkSemaphoreSubmitInfo &getRenderFinishedSemaphoreSubmitInfo() const;
		const VkSemaphoreSubmitInfo &getImageAvailableSemaphoreSubmitInfo() const;

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

#endif // VK_SWAPCHAIN_H_

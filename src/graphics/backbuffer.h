#ifndef VK_BACKBUFFER_H_
#define VK_BACKBUFFER_H_

#include <glm/vec4.hpp>

#include <vulkan/vulkan.h>

#include "generic_render_target.h"
#include "texture.h"
#include "util.h"
#include "render_pass_builder.h"

namespace llt
{
	class VulkanBackend;

	class Backbuffer : public GenericRenderTarget
	{
	public:
		Backbuffer();
		~Backbuffer() override;

		/*
		* Actually builds the backbuffer.
		*/
		void create();

		/*
		* Builds the surface of the backbuffer.
		*/
		void createSurface();

		/*
		* Creates the swap chain.
		*/
		void createSwapChain();
		void createSwapChainImageViews();

		/*
		* Functions called to clean up specific components of the backbuffer.
		*/
		void cleanUp() override;
		void cleanUpSwapChain();
		void cleanUpTextures();

		/*
		* Sets the clear for the colour and depth stencil
		*/
		void setClearColour(int idx, const Colour& colour) override;
		void setDepthStencilClear(float depth, uint32_t stencil) override;

		/*
		* Get the next image.
		*/
		void acquireNextImage();

		/*
		* Swap between the three buffers.
		* (Triple Buffering)
		*/
		void swapBuffers();

		/*
		* Returns the attachment at index 'idx'.
		*/
		const Texture* getAttachment(int idx) const override;

		/*
		* Get the depth component of the backbuffer.
		*/
		const Texture* getDepthAttachment() const override;

		/*
		* Called when the window resizes.
		*/
		void onWindowResize(int width, int height);

		/*
		* Return the surface that the backbuffer uses.
		*/
		VkSurfaceKHR getSurface() const;

		/*
		* Get the number of MSAA samples that are being taken
		* for anti-aliasing.
		*/
		int getMSAA() const override;

		/*
		* Return the current texture index in triple-buffering.
		* Ranges from 0->2.
		*/
		int getCurrentTextureIdx() const;

		/*
		* Get the different semaphores.
		*/
		const VkSemaphore& getRenderFinishedSemaphore() const;
		const VkSemaphore& getImageAvailableSemaphore() const;

	private:
		/*
		* Creates the resources.
		*/
		void createColourResources(); // coloured visual component
		void createDepthResources(); // depth component

		/*
		* Creates the semaphores.
		*/
		void createSwapChainSyncObjects();

		/*
		* Reconstructs the swapchain.
		* For instance, when the window is re-sized the swapchain has to be re-built.
		*/
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

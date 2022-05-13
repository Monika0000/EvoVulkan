//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_SWAPCHAIN_H
#define EVOVULKAN_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>

#include <EvoVulkan/Types/Base/VulkanObject.h>

namespace EvoVulkan::Types {
    class Device;
    class Surface;
    class CmdBuffer;

    typedef struct _SwapChainBuffers {
        VkImage     m_image;
        VkImageView m_view;
    } SwapChainBuffer;

    class Swapchain : public IVkObject {
    public:
        Swapchain(const Swapchain&) = delete;
    private:
        VkSwapchainKHR   m_swapchain       = VK_NULL_HANDLE;
        Device*          m_device          = nullptr;
        Surface*         m_surface         = nullptr;
        VkInstance       m_instance        = VK_NULL_HANDLE;

        VkPresentModeKHR m_presentMode     = VK_PRESENT_MODE_MAX_ENUM_KHR;

        VkFormat         m_depthFormat     = {};
        VkFormat         m_colorFormat     = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR  m_colorSpace      = VkColorSpaceKHR::VK_COLOR_SPACE_MAX_ENUM_KHR;

        //! note: images will be automatic destroyed after destroying swapchain
        VkImage*         m_swapchainImages = nullptr;
        uint32_t         m_countImages     = 0;

        SwapChainBuffer* m_buffers         = nullptr;

        uint32_t         m_surfaceWidth    = 0;
        uint32_t         m_surfaceHeight   = 0;

        bool             m_vsync           = false;
    private:
        bool InitFormats();

        bool CreateBuffers();
        void DestroyBuffers();

        bool CreateImages();
    private:
        Swapchain()  = default;
        ~Swapchain() = default;
    public:
        [[nodiscard]] bool IsReady() const override;

        bool SurfaceIsAvailable();

        bool ReSetup(uint32_t width, uint32_t height, uint32_t countImages);

        [[nodiscard]] SwapChainBuffer* GetBuffers()   const { return m_buffers;       }
        [[nodiscard]] uint32_t GetSurfaceWidth()      const { return m_surfaceWidth;  }
        [[nodiscard]] uint32_t GetSurfaceHeight()     const { return m_surfaceHeight; }
        [[nodiscard]] VkFormat GetDepthFormat()       const { return m_depthFormat;   }
        [[nodiscard]] VkFormat GetColorFormat()       const { return m_colorFormat;   }
        [[nodiscard]] VkColorSpaceKHR GetColorSpace() const { return m_colorSpace;    }
        [[nodiscard]] uint32_t GetCountImages()       const { return m_countImages;   }
    public:
        /**
        * Acquires the next image in the swap chain
        *
        * @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
        * @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
        *
        * @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
        *
        * @return VkResult of the image acquisition
        */
        VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex) const;

        /**
        * Queue an image for presentation
        *
        * @param queue Presentation queue for presenting the image
        * @param imageIndex Index of the swapchain image to queue for presentation
        * @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
        *
        * @return VkResult of the queue presentation
        */
        inline VkResult QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) const {
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pNext            = NULL;
            presentInfo.swapchainCount   = 1;
            presentInfo.pSwapchains      = &m_swapchain;
            presentInfo.pImageIndices    = &imageIndex;

            /// Check if a wait semaphore has been specified to wait for before presenting the image
            if (waitSemaphore != VK_NULL_HANDLE) {
                presentInfo.pWaitSemaphores    = &waitSemaphore;
                presentInfo.waitSemaphoreCount = 1;
            }

            try {
                return vkQueuePresentKHR(queue, &presentInfo);
            }
            catch (const std::exception& ex) {
                VK_ERROR("Swapchain::QueuePresent() : an exception has been occurred! \n\tMessage: " + std::string(ex.what()));
                return VK_ERROR_UNKNOWN;
            }
        }
    public:
        static Swapchain* Create(
                const VkInstance& instance,
                Surface* surface,
                Device* device,
                bool vsync,
                uint32_t width,
                uint32_t height,
                uint32_t imagesCount);

        void Destroy() override;
        void Free() override;
    };
}

#endif //EVOVULKAN_SWAPCHAIN_H

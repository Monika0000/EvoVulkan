//
// Created by Nikita on 12.04.2021.
//

#ifndef EVOVULKAN_VULKANKERNEL_H
#define EVOVULKAN_VULKANKERNEL_H

#include <EvoVulkan/Memory/Allocator.h>

#include <EvoVulkan/Tools/VulkanTools.h>
#include <EvoVulkan/Tools/VulkanInsert.h>
#include <EvoVulkan/Types/Instance.h>

#include <EvoVulkan/Types/VulkanBuffer.h>

#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/RenderPass.h>
#include <EvoVulkan/Complexes/Framebuffer.h>

#include <EvoVulkan/Types/MultisampleTarget.h>

namespace EvoVulkan::Core {
    enum class FrameResult : uint8_t {
        Error = 0, Success = 1, OutOfDate = 2, DeviceLost = 3
    };

    enum class RenderResult : uint8_t {
        Success = 0, Fatal = 1, Error = 2
    };

    class DLL_EVK_EXPORT VulkanKernel : public Tools::NonCopyable {
    protected:
        VulkanKernel() = default;
        ~VulkanKernel() override = default;

    public:
        bool PreInit(
                const std::string& appName,
                const std::string& engineName,
                const std::string& glslc,
                const std::vector<const char*>& instExtensions,
                const std::vector<const char*>& validationLayers);

        bool Init(
                const std::function<VkSurfaceKHR(const VkInstance&)>& platformCreate,
                void* windowHandle,
                const std::vector<const char*>& deviceExtensions, const bool& enableSampleShading,
                bool vsync);

        bool PostInit();

        FrameResult PrepareFrame();
        RenderResult NextFrame();
        FrameResult SubmitFrame();

    public:
        EVK_NODISCARD EVK_INLINE VkPipelineCache GetPipelineCache() const noexcept { return m_pipelineCache; }
        EVK_NODISCARD EVK_INLINE VkCommandBuffer* GetDrawCmdBuffs() const { return m_drawCmdBuffs; }
        EVK_NODISCARD EVK_INLINE Types::Device* GetDevice() const { return m_device; }
        EVK_NODISCARD EVK_INLINE Memory::Allocator* GetAllocator() const { return m_allocator; }
        EVK_NODISCARD EVK_INLINE Types::MultisampleTarget* GetMultisampleTarget() const { return m_multisample; }
        EVK_NODISCARD EVK_INLINE Types::CmdPool* GetCmdPool() const { return m_cmdPool; }
        EVK_NODISCARD EVK_INLINE Types::Swapchain* GetSwapchain() const { return m_swapchain; }
        EVK_NODISCARD EVK_INLINE Types::Surface* GetSurface() const { return m_surface; }
        EVK_NODISCARD EVK_INLINE VkInstance GetInstance() const { return *m_instance; }
        EVK_NODISCARD EVK_INLINE bool HasErrors() const noexcept { return m_hasErrors; }
        EVK_NODISCARD EVK_INLINE VkViewport GetViewport()   const noexcept { return Tools::Initializers::Viewport((float)m_width, (float)m_height, 0.0f, 1.0f); }
        EVK_NODISCARD EVK_INLINE VkRect2D   GetScissor()    const noexcept { return Tools::Initializers::Rect2D(m_width, m_height, 0, 0);                       }
        EVK_NODISCARD EVK_INLINE VkRect2D   GetRenderArea() const noexcept { return { VkOffset2D(), { m_width, m_height } };                                    }
        EVK_NODISCARD EVK_INLINE Types::RenderPass GetRenderPass() const noexcept { return m_renderPass; }
        EVK_NODISCARD EVK_INLINE VkFramebuffer* GetFrameBuffers() { return m_frameBuffers.data(); }
        EVK_NODISCARD EVK_INLINE bool MultisamplingEnabled() const noexcept { return m_multisampling; }

        EVK_NODISCARD Core::DescriptorManager* GetDescriptorManager() const;
        EVK_NODISCARD uint32_t GetCountBuildIterations() const;
        EVK_NODISCARD bool IsValidationLayersEnabled() const { return m_validationEnabled; }

        void SetFramebuffersQueue(const std::vector<Complexes::FrameBuffer*>& queue);
        void SetMultisampling(uint32_t sampleCount);
        void SetSwapchainImagesCount(uint32_t count);

        void SetGUIEnabled(bool enabled);

        bool SetValidationLayersEnabled(bool value);
        void SetSize(uint32_t width, uint32_t height);
        bool ResizeWindow();

    public:
        virtual bool BuildCmdBuffers() = 0;
        virtual bool Destroy();
        virtual bool OnResize() = 0;
        virtual bool OnComplete() { return true; }

    protected:
        virtual RenderResult Render() { return RenderResult::Fatal; }

    private:
        bool ReCreateFrameBuffers();
        bool ReCreateSynchronizations();
        void DestroyFrameBuffers();

    public:
        uint8_t                    m_countDCB             = 0;
        VkCommandBuffer*           m_drawCmdBuffs         = nullptr;
        std::vector<VkFramebuffer> m_frameBuffers         = std::vector<VkFramebuffer>();

    protected:
        std::mutex                 m_mutex                = std::mutex();

        bool                       m_multisampling        = false;
        bool                       m_hasErrors            = false;
        bool                       m_paused               = false;

        int32_t                    m_newWidth             = -1;
        int32_t                    m_newHeight            = -1;

        uint32_t                   m_width                = 0;
        uint32_t                   m_height               = 0;
        uint32_t                   m_swapchainImages      = 0;
        uint32_t                   m_sampleCount          = 1;

        Types::RenderPass          m_renderPass           = { };
        VkPipelineCache            m_pipelineCache        = VK_NULL_HANDLE;

        Types::Instance*           m_instance             = nullptr;
        Types::Device*             m_device               = nullptr;
        Memory::Allocator*         m_allocator            = nullptr;
        Types::Surface*            m_surface              = nullptr;
        Types::Swapchain*          m_swapchain            = nullptr;
        Types::CmdPool*            m_cmdPool              = nullptr;
        Types::MultisampleTarget*  m_multisample          = nullptr;

        Core::DescriptorManager*   m_descriptorManager    = nullptr;

        /// optional. Maybe nullptr
        VkSemaphore                m_waitSemaphore        = VK_NULL_HANDLE;
        Types::Synchronization     m_syncs                = {};
        VkSubmitInfo               m_submitInfo           = {};

        std::vector<VkFence>       m_waitFences           = std::vector<VkFence>();
        uint32_t                   m_currentBuffer        = 0;

        std::vector<VkSubmitInfo>  m_framebuffersQueue    = {};

        VkPipelineStageFlags       m_submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        bool                       m_GUIEnabled           = false;

    private:
        std::vector<const char*>   m_instExtensions       = {};
        std::vector<const char*>   m_validationLayers     = {};

        std::string                m_appName              = "Unknown";
        std::string                m_engineName           = "NoEngine";

        VkDebugUtilsMessengerEXT   m_debugMessenger       = VK_NULL_HANDLE;

        bool                       m_validationEnabled    = false;

        bool                       m_isPreInitialized     = false;
        bool                       m_isInitialized        = false;
        bool                       m_isPostInitialized    = false;

    };
}

#endif //EVOVULKAN_VULKANKERNEL_H

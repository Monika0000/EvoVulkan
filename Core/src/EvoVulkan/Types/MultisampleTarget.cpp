//
// Created by Nikita on 11.06.2021.
//

#include <EvoVulkan/Types/MultisampleTarget.h>
#include <EvoVulkan/Tools/VulkanTools.h>

EvoVulkan::Types::MultisampleTarget *EvoVulkan::Types::MultisampleTarget::Create(
        EvoVulkan::Types::Device *device,
        Swapchain* swapchain,
        uint32_t w, uint32_t h)
{
    auto multisample = new MultisampleTarget();
    multisample->m_device    = device;
    multisample->m_swapchain = swapchain;

    if (!multisample->ReCreate(w, h)) {
        VK_ERROR("MultisampleTarget::Create() : failed to re-create multisample!");
        return nullptr;
    }

    return multisample;
}

bool EvoVulkan::Types::MultisampleTarget::ReCreate(uint32_t w, uint32_t h) {
    this->Destroy();

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(*m_device, &deviceProperties);

    if (!((deviceProperties.limits.framebufferColorSampleCounts >= m_device->GetMSAASamples()) && (deviceProperties.limits.framebufferDepthSampleCounts >= m_device->GetMSAASamples()))) {
        VK_ERROR("MultisampleTarget::ReCreate() : multisampling is unsupported!");
        return false;
    }

    //! ----------------------------------- Color target -----------------------------------

    m_color.m_image = Tools::CreateImage(
            m_device, w, h, 1,
            m_swapchain->GetColorFormat(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
            &m_color.m_memory);
    if (m_color.m_image == VK_NULL_HANDLE) {
        VK_ERROR("MultisampleTarget::ReCreate() : failed to create color image!");
        return false;
    }

    m_color.m_view = Tools::CreateImageView(*m_device, m_color.m_image, m_swapchain->GetColorFormat(), 1, VK_IMAGE_ASPECT_COLOR_BIT);
    if (m_color.m_view == VK_NULL_HANDLE) {
        VK_ERROR("MultisampleTarget::ReCreate() : failed to create color image view!");
        return false;
    }

    //! ----------------------------------- Depth target -----------------------------------

    m_depth.m_image = Tools::CreateImage(
            m_device, w, h, 1,
            m_swapchain->GetDepthFormat(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
            &m_depth.m_memory);
    if (m_depth.m_image == VK_NULL_HANDLE) {
        VK_ERROR("MultisampleTarget::ReCreate() : failed to create depth image!");
        return false;
    }

    m_depth.m_view = Tools::CreateImageView(*m_device, m_depth.m_image, m_swapchain->GetDepthFormat(), 1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    if (m_depth.m_view == VK_NULL_HANDLE) {
        VK_ERROR("MultisampleTarget::ReCreate() : failed to create depth image view!");
        return false;
    }

    return true;
}

void EvoVulkan::Types::MultisampleTarget::Destroy() {
    // Destroy MSAA target
    if (m_color.m_image && m_color.m_view && m_color.m_memory) {
        vkDestroyImage(*m_device, m_color.m_image, nullptr);
        vkDestroyImageView(*m_device, m_color.m_view, nullptr);
        vkFreeMemory(*m_device, m_color.m_memory, nullptr);

        m_color.m_image = VK_NULL_HANDLE;
        m_color.m_view = VK_NULL_HANDLE;
        m_color.m_memory = VK_NULL_HANDLE;
    }

    if (m_depth.m_image && m_depth.m_view && m_depth.m_memory) {
        vkDestroyImage(*m_device, m_depth.m_image, nullptr);
        vkDestroyImageView(*m_device, m_depth.m_view, nullptr);
        vkFreeMemory(*m_device, m_depth.m_memory, nullptr);

        m_depth.m_image = VK_NULL_HANDLE;
        m_depth.m_view = VK_NULL_HANDLE;
        m_depth.m_memory = VK_NULL_HANDLE;
    }
}
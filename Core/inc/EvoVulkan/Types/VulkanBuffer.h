//
// Created by Nikita on 07.05.2021.
//

#ifndef EVOVULKAN_VULKANBUFFER_H
#define EVOVULKAN_VULKANBUFFER_H

#include <vector>
#include <vulkan/vulkan.h>

namespace EvoVulkan::Types {
    class Device;
    /**
    * @brief Encapsulates access to a Vulkan buffer backed up by device memory
    * @note To be filled by an external source like the VulkanDevice
    */
    struct Buffer {
        static Buffer* Create(
                const Device* device,
                VkBufferUsageFlags usageFlags,
                VkMemoryPropertyFlags memoryPropertyFlags,
                VkDeviceSize size, void *data = nullptr);

        VkDevice               m_device              = {};
        VkBuffer               m_buffer              = VK_NULL_HANDLE;
        VkDeviceMemory         m_memory              = VK_NULL_HANDLE;
        VkDescriptorBufferInfo m_descriptor          = {};
        VkDeviceSize           m_size                = 0;
        VkDeviceSize           m_alignment           = 0;
        void*                  m_mapped              = nullptr;
        /** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
        VkBufferUsageFlags     m_usageFlags          = {};
        /** @brief Memory property flags to be filled by external source at buffer creation (to query at some later point) */
        VkMemoryPropertyFlags  m_memoryPropertyFlags = {};

        VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void Unmap();

        VkResult Bind(VkDeviceSize offset = 0) const;

        void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

        void CopyTo(void *data, VkDeviceSize size);
        void CopyToDevice(void *data, VkDeviceSize size) const;

        VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

        void Destroy();
    };
}

#endif //EVOVULKAN_VULKANBUFFER_H
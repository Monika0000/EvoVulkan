//
// Created by Monika on 07.02.2022.
//

#ifndef EVOVULKAN_INSTANCE_H
#define EVOVULKAN_INSTANCE_H

#include <EvoVulkan/Tools/NonCopyable.h>

namespace EvoVulkan::Types {
    class DLL_EVK_EXPORT Instance : public Tools::NonCopyable {
        using StringVector = std::vector<const char*>;
    private:
        explicit Instance(uint32_t version)
            : m_instance(VK_NULL_HANDLE)
            , m_version(version)
        { }

        Instance()
            : Instance(UINT32_MAX)
        { }

        ~Instance() override = default;

    public:
        static Instance* Create(const std::string& appName, const std::string& engineName,
                StringVector extensions, const StringVector& layers, bool validationEnabled);

        operator VkInstance() const { return m_instance; }

    public:
        void Destroy();
        void Free();

        EVK_NODISCARD uint32_t GetVersion() const;
        EVK_NODISCARD bool Valid() const;

    private:
        VkInstance m_instance;
        uint32_t m_version;

    };
}

#endif //EVOVULKAN_INSTANCE_H

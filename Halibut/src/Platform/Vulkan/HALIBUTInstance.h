#pragma once
#include "Core/Core.h"
#include <vulkan/vulkan_raii.hpp>

namespace HALIBUT
{
    struct AppInfo
    {
        const std::string AppName = "Hello Halibut";
        vec3 AppVersion = {1, 0, 0};
        const std::string EngineName = "no Engine";
        vec3 EngineVersion  = {1, 0, 0};
        uint32_t ApiVersion = VK_API_VERSION_1_3;
    };

    class HALIBUT_API HALIBUTInstance
    {
    public:
        HALIBUTInstance();
        ~HALIBUTInstance();

        inline vk::raii::Instance& GetInstance() { return m_Instance; }
        inline vk::raii::Context& GetContext() { return m_Context; }
    private:
        std::vector<const char *> getRequiredInstanceExtensions(bool enableValidationLayers);
        void SetupDebugMessenger();
    private:
        vk::raii::Instance m_Instance = nullptr;
        vk::raii::Context  m_Context;
        const std::vector<char const *> m_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
        vk::raii::DebugUtilsMessengerEXT m_DebugMessenger  = nullptr;
        bool m_EnableValidationLayers = false;
        AppInfo m_AppInfo;
    };
}

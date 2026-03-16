#include "halibutpch.h"
#include "HALIBUTInstance.h"
#include <GLFW/glfw3.h>
namespace HALIBUT
{

#ifdef NDEBUG
    constexpr bool kDefaultEnableValidationLayers = false;
#else
    constexpr bool kDefaultEnableValidationLayers = true;
#endif

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT       severity,
        VkDebugUtilsMessageTypeFlagsEXT              type,
        const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
        void *                                       pUserData)
    {
        (void)pUserData;

        // Avoid flooding terminal/WSL when a warning is emitted every frame.
        static std::map<int32_t, uint32_t> messageCounts;
        const int32_t messageId = pCallbackData != nullptr ? pCallbackData->messageIdNumber : 0;
        uint32_t& count = messageCounts[messageId];
        ++count;
        if (count > 3)
        {
            if (count == 4)
            {
                std::cerr << "validation layer: messageId=" << messageId
                          << " 重复过多，后续同类日志已抑制。" << std::endl;
            }
            return VK_FALSE;
        }

        const char* severityText = "INFO";
        if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
        {
            severityText = "ERROR";
        }
        else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
        {
            severityText = "WARN";
        }

        const char* typeText = "General";
        if ((type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0)
        {
            typeText = "Validation";
        }
        else if ((type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0)
        {
            typeText = "Performance";
        }

        std::cerr << "VALIDATION LAYER INFO[" << severityText << "/" << typeText << "]: "
                  << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    HALIBUTInstance::HALIBUTInstance()
    {
        m_EnableValidationLayers = kDefaultEnableValidationLayers;
        if (const char* disableValidation = std::getenv("HALIBUT_DISABLE_VALIDATION");
            disableValidation != nullptr && disableValidation[0] != '\0' && std::string(disableValidation) != "0")
        {
            m_EnableValidationLayers = false;
        }
        if (const char* forceValidation = std::getenv("HALIBUT_FORCE_VALIDATION");
            forceValidation != nullptr && forceValidation[0] != '\0' && std::string(forceValidation) != "0")
        {
            m_EnableValidationLayers = true;
        }

        vk::ApplicationInfo appInfo{
            .pApplicationName   = m_AppInfo.AppName.c_str(),
            .applicationVersion = VK_MAKE_VERSION(m_AppInfo.AppVersion.x, m_AppInfo.AppVersion.y, m_AppInfo.AppVersion.z),
            .pEngineName        = m_AppInfo.EngineName.c_str(),
            .engineVersion      = VK_MAKE_VERSION(m_AppInfo.EngineVersion.x,m_AppInfo.EngineVersion.y,m_AppInfo.EngineVersion.z),
            .apiVersion         = m_AppInfo.ApiVersion
        };

		// 获取验证层列表
        std::vector<char const*> requiredLayers;
        if (m_EnableValidationLayers)
        {
            requiredLayers.assign(m_ValidationLayers.begin(), m_ValidationLayers.end());
        }

        // 检查验证层是否被Vulkan支持
        auto layerProperties = m_Context.enumerateInstanceLayerProperties();
		auto unsupportedLayerIt = std::ranges::find_if(
            requiredLayers,
            [&layerProperties](auto const &requiredLayer) {
                return std::ranges::none_of(
                    layerProperties,
                    [requiredLayer](auto const &layerProperty) { 
                        return strcmp(layerProperty.layerName, requiredLayer) == 0; 
                    }
                );
            }
        );
        if (unsupportedLayerIt != requiredLayers.end())
        {
            std::cerr << "[WARN] 验证层不可用，继续运行但禁用验证层: " << *unsupportedLayerIt << std::endl;
            requiredLayers.clear();
            m_EnableValidationLayers = false;
        }
        // 获取实例所需的拓展列表
		auto requiredExtensions = getRequiredInstanceExtensions(m_EnableValidationLayers);

		// 检查实例所需的拓展是否被Vulkan支持
		auto extensionProperties = m_Context.enumerateInstanceExtensionProperties();
		auto unsupportedPropertyIt =
		    std::ranges::find_if(
                requiredExtensions,
                [&extensionProperties](auto const &requiredExtension) {
                    return std::ranges::none_of(
                        extensionProperties,
                        [requiredExtension](auto const &extensionProperty) { 
                            return strcmp(extensionProperty.extensionName, requiredExtension) == 0; 
                        }
                    );
                }
            );

		if (unsupportedPropertyIt != requiredExtensions.end())
		{
			throw std::runtime_error("实例要求的拓展不被支持: " + std::string(*unsupportedPropertyIt));
		}

        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames     = requiredLayers.data(),
            .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()
        };
        m_Instance = vk::raii::Instance(m_Context, createInfo);
        SetupDebugMessenger();
    }
    HALIBUTInstance::~HALIBUTInstance()
    {
    }

    std::vector<const char *> HALIBUTInstance::getRequiredInstanceExtensions(bool enableValidationLayers)
    {
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }

    void HALIBUTInstance::SetupDebugMessenger()
	{
		if (!m_EnableValidationLayers)
			return;
        // TODO: 可抽象debug消息配置
		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | 
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType     = messageTypeFlags,
            .pfnUserCallback = reinterpret_cast<decltype(vk::DebugUtilsMessengerCreateInfoEXT{}.pfnUserCallback)>(&debugCallback)};
		m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}
}

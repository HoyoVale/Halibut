#pragma once
#include "Core.h"
#include "Window.h"
#include "Platform/Vulkan/HALIBUTInstance.h"
#include "Platform/Vulkan/HALIBUTDevice.h"
#include "Platform/Vulkan/HALIBUTSwapchain.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_raii.hpp>

namespace HALIBUT
{
    class HALIBUT_API ApplicationImGuiContext
    {
    public:
        ApplicationImGuiContext() = default;
        ~ApplicationImGuiContext() = default;

        void LoadUiSettings();
        void SaveUiSettings() const;

        void Initialize(Window& window, HALIBUTInstance& instance, HALIBUTDevice& device, HALIBUTSwapchain& swapchain);
        void RecreateBackends(Window& window, HALIBUTInstance& instance, HALIBUTDevice& device, HALIBUTSwapchain& swapchain);
        void Shutdown();
        void BeginFrame();
        void Render(vk::raii::CommandBuffer& commandBuffer);

        float GetFontScale() const;
        void SetFontScale(float scale);
        int GetFontPresetIndex() const;
        void SetFontPresetIndex(int presetIndex);
        const std::vector<std::string>& GetFontPresetNames() const;

    private:
        void InitPlatformBackend(Window& window);
        void InitVulkanBackend(HALIBUTInstance& instance, HALIBUTDevice& device, HALIBUTSwapchain& swapchain);
        void ShutdownPlatformBackend();
        void ShutdownVulkanBackend();
        void SetupFonts();

    private:
        vk::raii::DescriptorPool m_DescriptorPool = nullptr;
        std::vector<ImFont*> m_Fonts;
        std::vector<std::string> m_FontPresetNames;
        int m_FontPresetIndex = 0;
        float m_FontScale = 1.0f;
        bool m_HasPersistedFontPreset = false;
        bool m_IsInitialized = false;
        std::string m_UiSettingsPath = "editor_ui_settings.ini";
    };
}

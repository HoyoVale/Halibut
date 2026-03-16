#pragma once
#include "Core/Core.h"
#include "Core/Window.h"
#include "HALIBUTDevice.h"
#include "HALIBUTSurface.h"
#include <vulkan/vulkan_raii.hpp>

namespace HALIBUT
{
    class HALIBUT_API HALIBUTSwapchain
    {
    public:
        HALIBUTSwapchain(
            HALIBUTDevice& device,
            HALIBUTSurface& surface,
            Window& window
        );
        ~HALIBUTSwapchain();
        inline vk::raii::SwapchainKHR& GetSwapchain() { return m_SwapChain; }
        inline std::vector<vk::Image>& GetSwapChainImages() { return m_SwapChainImages; }
        inline const vk::Extent2D& GetSwapChainExtent() { return m_SwapChainExtent; }
        inline vk::SurfaceFormatKHR& GetSwapchainSurfaceFormat() { return m_SwapChainSurfaceFormat; }
        inline std::vector<vk::raii::ImageView>& GetImageViews() { return m_SwapChainImageViews; }
    private:
        uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
        vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
        vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes, bool vsyncEnabled);
        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    private:
        Window&                     m_Window;
        HALIBUTDevice&                  m_Device;
        HALIBUTSurface&                 m_Surface;

        vk::raii::SwapchainKHR      m_SwapChain = nullptr;
        vk::SurfaceFormatKHR        m_SwapChainSurfaceFormat;
        std::vector<vk::Image>      m_SwapChainImages;
        vk::Extent2D                m_SwapChainExtent;
        std::vector<vk::raii::ImageView> m_SwapChainImageViews;
        
    };
}

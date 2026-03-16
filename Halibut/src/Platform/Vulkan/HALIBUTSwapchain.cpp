#include "halibutpch.h"
#include "HALIBUTSwapchain.h"
#include <GLFW/glfw3.h>

namespace HALIBUT
{
    HALIBUTSwapchain::HALIBUTSwapchain(
        HALIBUTDevice& device,
        HALIBUTSurface& surface,
        Window& window
    )
        :m_Window(window),
        m_Device(device),
        m_Surface(surface)
    {
        auto surfaceCapabilities = (*m_Device.GetPhysicalDevice()).getSurfaceCapabilitiesKHR(*m_Surface.GetSurface());
        if (!m_Device.GetQueueFamilyIndices().IsComplete())
        {
            throw std::runtime_error("Could not find queue families for swapchain");
        }

		m_SwapChainExtent          = chooseSwapExtent(surfaceCapabilities);
		m_SwapChainSurfaceFormat   = chooseSwapSurfaceFormat((*m_Device.GetPhysicalDevice()).getSurfaceFormatsKHR(*m_Surface.GetSurface()));

        // Re-query surface capabilities just before creating swapchain.
        // On some WSL/X11 setups, extent can change during initial window setup.
        surfaceCapabilities = (*m_Device.GetPhysicalDevice()).getSurfaceCapabilitiesKHR(*m_Surface.GetSurface());
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            if (m_SwapChainExtent.width != surfaceCapabilities.currentExtent.width ||
                m_SwapChainExtent.height != surfaceCapabilities.currentExtent.height)
            {
                std::cout << "[HALIBUT] Adjust swapchain extent from "
                          << m_SwapChainExtent.width << "x" << m_SwapChainExtent.height
                          << " to fixed currentExtent "
                          << surfaceCapabilities.currentExtent.width << "x" << surfaceCapabilities.currentExtent.height
                          << std::endl;
            }
            m_SwapChainExtent = surfaceCapabilities.currentExtent;
        }
        else
        {
            m_SwapChainExtent = vk::Extent2D{
                std::clamp(m_SwapChainExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
                std::clamp(m_SwapChainExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
            };
        }

		vk::SwapchainCreateInfoKHR swapChainCreateInfo{ // 可自定义？
            .surface          = *m_Surface.GetSurface(),
            .minImageCount    = chooseSwapMinImageCount(surfaceCapabilities),
            .imageFormat      = m_SwapChainSurfaceFormat.format,
            .imageColorSpace  = m_SwapChainSurfaceFormat.colorSpace,
            .imageExtent      = m_SwapChainExtent,
            .imageArrayLayers = 1,
            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .preTransform     = surfaceCapabilities.currentTransform,
            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode      = chooseSwapPresentMode((*m_Device.GetPhysicalDevice()).getSurfacePresentModesKHR(*m_Surface.GetSurface()), m_Window.IsVSync()),
            .clipped          = true};

        std::array<uint32_t, 2> queueFamilyIndicesArray = {
            m_Device.GetQueueFamilyIndices().graphicsFamily.value(),
            m_Device.GetQueueFamilyIndices().presentFamily.value()
        };
        if (m_Device.GetQueueFamilyIndices().graphicsFamily != m_Device.GetQueueFamilyIndices().presentFamily) // 可自定义？
        {
            swapChainCreateInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
            swapChainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndicesArray.size());
            swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndicesArray.data();
        }

		m_SwapChain = vk::raii::SwapchainKHR(m_Device.GetDevice(), swapChainCreateInfo);
        m_SwapChainImages = m_SwapChain.getImages();

        assert(m_SwapChainImageViews.empty());

        vk::ImageViewCreateInfo imageViewCreateInfo{ // 可自定义？
            .viewType = vk::ImageViewType::e2D, 
            .format = m_SwapChainSurfaceFormat.format, 
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        };
        for (auto& image : m_SwapChainImages)
        {
            imageViewCreateInfo.image = image;
            m_SwapChainImageViews.emplace_back(m_Device.GetDevice(), imageViewCreateInfo);
        }
    }

    HALIBUTSwapchain::~HALIBUTSwapchain()
    {
    }

    uint32_t HALIBUTSwapchain::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities)
	{
		auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount); // 是否能自定义
		if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
		{
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

    vk::SurfaceFormatKHR HALIBUTSwapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    vk::PresentModeKHR HALIBUTSwapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes, bool vsyncEnabled) {
        if (vsyncEnabled)
        {
            return vk::PresentModeKHR::eFifo;
        }

        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eImmediate;
    }

    vk::Extent2D HALIBUTSwapchain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        int width, height;
        glfwGetFramebufferSize((GLFWwindow*)m_Window.GetNativeWindow(), &width, &height);

        const uint32_t framebufferWidth = static_cast<uint32_t>(std::max(width, 1));
        const uint32_t framebufferHeight = static_cast<uint32_t>(std::max(height, 1));

        return {
            std::clamp(framebufferWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(framebufferHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }
}

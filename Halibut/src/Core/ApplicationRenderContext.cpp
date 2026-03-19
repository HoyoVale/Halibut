#include "halibutpch.h"
#include "ApplicationRenderContext.h"
#include <GLFW/glfw3.h>

namespace HALIBUT
{
    ApplicationRenderContext::ApplicationRenderContext(Window& window)
        : m_Window(window)
    {
        m_Instance = MakeUPtr<HALIBUTInstance>();
        m_Surface = MakeUPtr<HALIBUTSurface>(m_Window, *m_Instance);
        m_Device = MakeUPtr<HALIBUTDevice>(*m_Instance, *m_Surface);
        CreateSwapchainResources();
    }

    ApplicationRenderContext::~ApplicationRenderContext() = default;

    void ApplicationRenderContext::WaitIdle() const
    {
        if (m_Device != nullptr)
        {
            m_Device->GetDevice().waitIdle();
        }
    }

    void ApplicationRenderContext::RecreateSwapchain()
    {
        WaitForValidFramebufferSize();
        WaitIdle();
        if (m_Renderer != nullptr)
        {
            m_ClearColor = m_Renderer->GetClearColor();
        }
        DestroySwapchainResources();
        CreateSwapchainResources();
        // if (m_Camera != nullptr)
        // {
        //     m_Camera->SetViewportExtent(m_Swapchain->GetSwapChainExtent());
        // }
    }

    void ApplicationRenderContext::WaitForValidFramebufferSize() const
    {
        int width = 0;
        int height = 0;
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(
                static_cast<GLFWwindow*>(m_Window.GetNativeWindow()),
                &width,
                &height
            );
            glfwWaitEvents();
        }
    }

    void ApplicationRenderContext::CreateSwapchainResources()
    {
        m_Swapchain = MakeUPtr<HALIBUTSwapchain>(*m_Device, *m_Surface, m_Window);
        m_Renderer = MakeUPtr<HALIBUTRenderer>(*m_Device, *m_Swapchain);
        m_Renderer->SetClearColor(m_ClearColor);
    }

    void ApplicationRenderContext::DestroySwapchainResources()
    {
        m_Renderer.reset();
        m_Swapchain.reset();
    }
}

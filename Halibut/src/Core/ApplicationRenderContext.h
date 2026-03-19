#pragma once
#include "Core.h"
#include "Window.h"
#include "Platform/Vulkan/HALIBUTInstance.h"
#include "Platform/Vulkan/HALIBUTSurface.h"
#include "Platform/Vulkan/HALIBUTDevice.h"
#include "Platform/Vulkan/HALIBUTSwapchain.h"
#include "Platform/Vulkan/HALIBUTRenderer.h"
#include <glm/glm.hpp>
namespace HALIBUT
{
    class HALIBUT_API ApplicationRenderContext
    {
    public:
        explicit ApplicationRenderContext(Window& window);
        ~ApplicationRenderContext();

        void WaitIdle() const;
        void RecreateSwapchain();

        HALIBUTInstance& GetInstance() const { return *m_Instance; }
        HALIBUTSurface& GetSurface() const { return *m_Surface; }
        HALIBUTDevice& GetDevice() const { return *m_Device; }
        HALIBUTSwapchain& GetSwapchain() const { return *m_Swapchain; }
        HALIBUTRenderer& GetRenderer() const { return *m_Renderer; }

    private:
        void WaitForValidFramebufferSize() const;
        void CreateSwapchainResources();
        void DestroySwapchainResources();

    private:
        Window& m_Window;
        UPtr<HALIBUTInstance> m_Instance;
        UPtr<HALIBUTSurface> m_Surface;
        UPtr<HALIBUTDevice> m_Device;
        UPtr<HALIBUTSwapchain> m_Swapchain;
        UPtr<HALIBUTRenderer> m_Renderer;
        glm::vec4 m_ClearColor = {0.1f, 0.2f, 0.3f, 1.0f};
    };
}

#pragma once
#include "Core/Core.h"
#include "Core/Window.h"
#include "HALIBUTInstance.h"
#include <vulkan/vulkan_raii.hpp>

namespace HALIBUT
{
    class HALIBUT_API HALIBUTSurface
    {
    public:
        HALIBUTSurface(
            Window& window,
            HALIBUTInstance& instance
        );
        ~HALIBUTSurface();
        inline vk::raii::SurfaceKHR& GetSurface() { return m_Surface; }
    private:

    private:
        Window&         m_Window;
        HALIBUTInstance&    m_Instance;
        vk::raii::SurfaceKHR m_Surface  = nullptr;
    };
}

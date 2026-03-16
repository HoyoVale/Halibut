#include "halibutpch.h"
#include "HALIBUTSurface.h"
#include <GLFW/glfw3.h>

namespace HALIBUT
{
    HALIBUTSurface::HALIBUTSurface(
        Window& window,
        HALIBUTInstance& instance
    )
        :m_Window(window),
        m_Instance(instance)
    {
        VkSurfaceKHR  _surface;
        if (glfwCreateWindowSurface(*(m_Instance.GetInstance()), (GLFWwindow*)m_Window.GetNativeWindow(), nullptr, &_surface) != 0) 
        {
            throw std::runtime_error("failed to create window surface!");
        }
        m_Surface = vk::raii::SurfaceKHR(m_Instance.GetInstance(), _surface);
    }
    HALIBUTSurface::~HALIBUTSurface()
    {
    }
}

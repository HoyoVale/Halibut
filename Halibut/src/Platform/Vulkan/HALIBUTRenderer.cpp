#include "halibutpch.h"
#include "HALIBUTRenderer.h"

namespace HALIBUT
{
    HALIBUTRenderer::HALIBUTRenderer(HALIBUTDevice &device, HALIBUTSwapchain &swapchain)
        :m_Device(device), m_Swapchain(swapchain)
    {

    }
    HALIBUTRenderer::~HALIBUTRenderer()
    {
        
    }
    void HALIBUTRenderer::BeginFrame()
    {
    }
    void HALIBUTRenderer::EndFrame()
    {
    }
    bool HALIBUTRenderer::IsNeedRecreationSwapchain()
    {
        return false;
    }
    void HALIBUTRenderer::SetClearColor(glm::vec4& clearcolor)
    {
        m_ClearColor = clearcolor;
    }
}
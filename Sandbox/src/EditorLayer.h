#pragma once
#include "Halibut.h"
#include <glm/glm.hpp>
namespace HALIBUT
{
class EditorLayer : public HALIBUT::Layer
{
public:
    EditorLayer();
    ~EditorLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(HALIBUT::Timestep ts) override;
    void OnImGuiRender() override;
    void OnSwapchainRecreated(HALIBUTDevice& device, HALIBUTSwapchain& swapchain, HALIBUTRenderer& renderer) override;

private:
    HALIBUTRenderer* m_Renderer = nullptr;
    glm::vec4 m_ClearColor = {0.1f,0.1f,0.1f,1.0f};
};
}

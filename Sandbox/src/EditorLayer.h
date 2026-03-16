#pragma once

#include "Halibut.h"
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

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
  
};
}

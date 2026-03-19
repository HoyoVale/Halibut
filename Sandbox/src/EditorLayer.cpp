#include "halibutpch.h"
#include "EditorLayer.h"
#include "imgui.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <nlohmann/json.hpp>

namespace HALIBUT
{

EditorLayer::EditorLayer()
{
}

void EditorLayer::OnAttach()
{
    Application* app = Application::Get();
    if (app == nullptr)
    {
        throw std::runtime_error("EditorLayer requires a valid Application instance");
    }
    m_Renderer = &app->GetRenderer();
    m_Renderer->SetClearColor(m_ClearColor);
}

void EditorLayer::OnDetach()
{
    Application* app = Application::Get();
}

void EditorLayer::OnUpdate(HALIBUT::Timestep ts)
{
    m_Renderer->BeginSwapchainPass();
    m_Renderer->EndSwapchainPass();
}

void EditorLayer::OnImGuiRender()
{

}

void EditorLayer::OnSwapchainRecreated(HALIBUTDevice& device, HALIBUTSwapchain& swapchain, HALIBUTRenderer& renderer)
{
    (void)device;
    (void)swapchain;
    m_Renderer = &renderer;
    m_Renderer->SetClearColor(m_ClearColor);
}
}
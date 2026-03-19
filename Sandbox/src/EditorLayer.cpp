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

    m_Pipeline = MakeUPtr<HALIBUTGraphicPipeline>(
        "assets/shaders/triangle.vert.spv",
        "assets/shaders/triangle.frag.spv",
        app->GetDevice(),
        app->GetSwapchain());
}

void EditorLayer::OnDetach()
{
}

void EditorLayer::OnUpdate(HALIBUT::Timestep ts)
{
    m_Renderer->BeginSwapchainPass();
    m_Renderer->BindPipeline(*m_Pipeline);
    m_Renderer->Draw(3, 1, 0, 0);
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
    Application* app = Application::Get();
    if (app == nullptr)
    {
        throw std::runtime_error("EditorLayer requires a valid Application instance");
    }
    m_Pipeline = MakeUPtr<HALIBUTGraphicPipeline>(
        "assets/shaders/triangle.vert.spv",
        "assets/shaders/triangle.frag.spv",
        app->GetDevice(),
        app->GetSwapchain());
}
}
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
namespace
{
    struct TriangleVertex
    {
        glm::vec2 Position;
        glm::vec3 Color;
    };

    const std::array<TriangleVertex, 4> kTriangleVertices = {
        TriangleVertex{{-0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }},
        TriangleVertex{{ 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }},
        TriangleVertex{{ 0.5f, 0.5f  }, { 0.0f, 0.0f, 1.0f }},
        TriangleVertex{{-0.5f, 0.5f  }, { 0.0f, 0.0f, 1.0f }}
    };

    const std::array<uint32_t, 6> kTriangleIndices = {
        0, 1, 2, 2, 3, 0 
    };

    HALIBUTVertexLayout CreateTriangleVertexLayout()
    {
        HALIBUTVertexLayout layout;
        layout.ReserveBindings(1).ReserveAttributes(2);
        layout.AddBinding<TriangleVertex>(0);
        layout.AddAttribute(0, 0, &TriangleVertex::Position);
        layout.AddAttribute(1, 0, &TriangleVertex::Color);
        return layout;
    }
}

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
    CreateMeshBuffer(app->GetDevice());
    CreatePipeline(app->GetDevice(), app->GetSwapchain());
}

void EditorLayer::OnDetach()
{
}

void EditorLayer::OnUpdate(HALIBUT::Timestep ts)
{
    (void)ts;
    m_Renderer->BeginSwapchainPass();
    m_Renderer->BindPipeline(*m_Pipeline);
    m_MeshBuffer->Bind(*m_Renderer);
    m_MeshBuffer->Draw(*m_Renderer);
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
    CreatePipeline(app->GetDevice(), app->GetSwapchain());
}

void EditorLayer::CreateMeshBuffer(HALIBUTDevice& device)
{
    HALIBUTMeshBufferDesc meshBufferDesc{};
    meshBufferDesc.VertexData = kTriangleVertices.data();
    meshBufferDesc.VertexDataSize = sizeof(kTriangleVertices);
    meshBufferDesc.VertexStride = static_cast<uint32_t>(sizeof(TriangleVertex));
    meshBufferDesc.IndexData = kTriangleIndices.data();
    meshBufferDesc.IndexDataSize = sizeof(kTriangleIndices);
    meshBufferDesc.IndexCount = static_cast<uint32_t>(kTriangleIndices.size());
    meshBufferDesc.IndexType = vk::IndexType::eUint32;

    m_MeshBuffer = MakeUPtr<HALIBUTMeshBuffer>(device, meshBufferDesc);
}

void EditorLayer::CreatePipeline(HALIBUTDevice& device, HALIBUTSwapchain& swapchain)
{
    const HALIBUTVertexLayout vertexLayout = CreateTriangleVertexLayout();
    m_Pipeline = MakeUPtr<HALIBUTGraphicPipeline>(
        "assets/shaders/triangle.vert.spv",
        "assets/shaders/triangle.frag.spv",
        device,
        swapchain,
        vertexLayout
    );
}
}

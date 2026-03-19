#include "halibutpch.h"
#include "Application.h"
#include "Core.h"
#include <GLFW/glfw3.h>

namespace HALIBUT
{
    Application* Application::s_App = nullptr;

    Application::Application(const std::string& name, bool enableImGui)
        // : m_ImGuiEnabled(enableImGui)
    {
        assert(!s_App && "Application already exists!");
        std::cout << "HoyoVK Engine v0.1" << std::endl;
        s_App = this;

        m_Window = UPtr<Window>(Window::Create(WindowProps(name)));
        m_Window->SetEventCallback(HALIBUT_BIND_EVENT_FN(Application::OnEvent));
        m_Window->SetVSync(false);

        m_RenderContext = MakeUPtr<ApplicationRenderContext>(*m_Window);

        // if (m_ImGuiEnabled)
        // {
        //     m_ImGuiContext = MakeUPtr<ApplicationImGuiContext>();
        //     m_ImGuiContext->Initialize(
        //         *m_Window,
        //         m_RenderContext->GetInstance(),
        //         m_RenderContext->GetDevice(),
        //         m_RenderContext->GetSwapchain()
        //     );
        // }
    }

    Application::~Application()
    {
        if (m_RenderContext != nullptr)
        {
            m_RenderContext->WaitIdle();
        }

        // if (m_ImGuiEnabled && m_ImGuiContext != nullptr)
        // {
        //     m_ImGuiContext->Shutdown();
        // }

        m_LayerStack.Clear();
        //m_ImGuiContext.reset();
        m_RenderContext.reset();
        m_Window.reset();
        glfwTerminate();
    }

    void Application::Run()
    {
        while (m_Running)
        {
            m_Window->OnUpdate();
            if (!m_Running)
            {
                break;
            }

            const float time = static_cast<float>(glfwGetTime());
            const Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            if (m_Minimized)
            {
                continue;
            }

            if (m_PendingSwapchainRecreate)
            {
                RecreateSwapchain();
                continue;
            }

            HALIBUTRenderer& renderer = m_RenderContext->GetRenderer();
            renderer.BeginFrame();
            if (renderer.IsNeedRecreationSwapchain())
            {
                m_PendingSwapchainRecreate = true;
                continue;
            }

            for (const UPtr<Layer>& layer : m_LayerStack)
            {
                layer->OnUpdate(timestep);
            }

            // if (m_ImGuiEnabled && m_ImGuiContext != nullptr)
            // {
            //     m_ImGuiContext->BeginFrame();

            //     for (const UPtr<Layer>& layer : m_LayerStack)
            //     {
            //         layer->OnImGuiRender();
            //     }

            //     ImGui::Render();

            //     HALIBUTRenderGraph renderGraph;
            //     renderGraph.AddSwapchainPass("ImGuiSwapchain", [&](HALIBUTRenderer& passRenderer) {
            //         m_ImGuiContext->Render(passRenderer.GetCommandBuffer());
            //     });
            //     renderer.ExecuteRenderGraph(renderGraph);
            // }

            renderer.EndFrame();
            if (renderer.IsNeedRecreationSwapchain())
            {
                m_PendingSwapchainRecreate = true;
            }
        }
    }

    void Application::OnEvent(Event& e)
    {
        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            (*--it)->OnEvent(e);
            if (e.Handled())
            {
                break;
            }
        }

        if (e.Handled())
        {
            return;
        }

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(HALIBUT_BIND_EVENT_FN(Application::OnWindowClose));
        if (!e.Handled())
        {
            dispatcher.Dispatch<WindowResizeEvent>(HALIBUT_BIND_EVENT_FN(Application::OnWindowResize));
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        (void)e;
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        m_Minimized = false;
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }

        m_PendingSwapchainRecreate = true;
        return false;
    }

    void Application::PushLayer(UPtr<Layer> layer)
    {
        layer->OnAttach();
        m_LayerStack.PushLayer(std::move(layer));
    }

    void Application::PushOverlay(UPtr<Layer> overlay)
    {
        overlay->OnAttach();
        m_LayerStack.PushOverlay(std::move(overlay));
    }

    void Application::NotifySwapchainRecreated()
    {
        for (const UPtr<Layer>& layer : m_LayerStack)
        {
            layer->OnSwapchainRecreated(
                m_RenderContext->GetDevice(),
                m_RenderContext->GetSwapchain(),
                m_RenderContext->GetRenderer()
            );
        }
    }

    void Application::RecreateSwapchain()
    {
        m_RenderContext->RecreateSwapchain();

        // if (m_ImGuiEnabled && m_ImGuiContext != nullptr)
        // {
        //     m_ImGuiContext->RecreateBackends(
        //         *m_Window,
        //         m_RenderContext->GetInstance(),
        //         m_RenderContext->GetDevice(),
        //         m_RenderContext->GetSwapchain()
        //     );
        // }

        NotifySwapchainRecreated();
        m_PendingSwapchainRecreate = false;
    }
}
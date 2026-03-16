#pragma once
#include "Core.h"
#include "Window.h"
#include "Event/Event.h"
#include "Event/ApplicationEvent.h"
#include "ApplicationRenderContext.h"
#include "LayerStack.h"

namespace HALIBUT
{
    class ApplicationRenderContext;
    class ApplicationImGuiContext;

    class HALIBUT_API Application
    {
    public:
        Application(const std::string& name = "HALIBUT Application", bool enableImGui = false);
        virtual ~Application();

        virtual void Run();
        void OnEvent(Event& e);

        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

        void PushLayer(UPtr<Layer> layer);
        void PushOverlay(UPtr<Layer> overlay);

        inline static Application* Get() { return s_App; }
        inline Window* GetWindow() { return m_Window.get(); }
        inline const Window* GetWindow() const { return m_Window.get(); }
        inline void RequestClose() { m_Running = false; }

    private:
        void NotifySwapchainRecreated();
        void RecreateSwapchain();

    private:
        static Application* s_App;

        float m_LastFrameTime = 0.0f;
        bool m_Running = true;
        bool m_Minimized = false;
        bool m_PendingSwapchainRecreate = false;
        bool m_ImGuiEnabled = false;

        LayerStack m_LayerStack;
        UPtr<Window> m_Window;

        UPtr<ApplicationRenderContext> m_RenderContext;
        //UPtr<ApplicationImGuiContext> m_ImGuiContext;
    };

    Application* CreateApplication();
}
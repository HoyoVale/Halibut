#pragma once
#include "Core/Core.h"
#include "Core/Window.h"
#include <GLFW/glfw3.h>

namespace HALIBUT
{
    class HALIBUT_API WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& props);
        virtual ~WindowsWindow();

        void OnUpdate() override;

        inline uint32_t GetWidth() const override { return m_Data.Width; }
        inline uint32_t GetHeight() const override { return m_Data.Height; }

        virtual void* GetNativeWindow() const override { return m_Window; }

        // Window attributes
		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

    private:
        virtual void Init(const WindowProps& props);
		virtual void Shutdown();
    private:
        struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
            bool VSync;

			EventCallbackFn EventCallback;
		};
		WindowData m_Data;

        GLFWwindow* m_Window;
    };
}

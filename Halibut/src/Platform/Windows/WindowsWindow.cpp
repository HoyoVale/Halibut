#include "halibutpch.h"
#include "WindowsWindow.h"
#include "Event/ApplicationEvent.h"
#include "Event/MouseEvent.h"
#include "Event/KeyEvent.h"

namespace HALIBUT
{
    static bool s_GLFWInitialized = false;
	static void GLFWErrorCallback(int error, const char* description)
	{
		//HG_CORE_ERROR("GLFW ERROR ({0}): {1}", error, description);
        std::cout << "GLFW ERROR (" << error << "): " << description << std::endl;
	}

	Window* Window::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;
		m_Data.VSync = false;
		m_Data.EventCallback = [](Event&) {};

		//HG_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);
        std::cout << "Creating window " << props.Title << " (" << props.Width << ", " << props.Height << ")" << std::endl;
		if (!s_GLFWInitialized)
		{
			// TODO: glfwTerminate on system shutdown
			int success = glfwInit();
			if (success == GLFW_FALSE)
			{
				const char* errorMessage = nullptr;
				const int errorCode = glfwGetError(&errorMessage);
				throw std::runtime_error(
					"Could not initialize GLFW, error " + std::to_string(errorCode) +
					": " + (errorMessage != nullptr ? std::string(errorMessage) : std::string("unknown"))
				);
			}
			glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
		if (m_Window == nullptr)
		{
			const char* errorMessage = nullptr;
			const int errorCode = glfwGetError(&errorMessage);
			throw std::runtime_error(
				"glfwCreateWindow failed, error " + std::to_string(errorCode) +
				": " + (errorMessage != nullptr ? std::string(errorMessage) : std::string("unknown"))
			);
		}

		glfwSetWindowUserPointer(m_Window, &m_Data);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});
		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				
				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
				}
			});
		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int c)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				KeyTypedEvent event(c);
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}

			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});

	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		//glfwSwapBuffers(m_Window); this function is for opengl
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		// Vulkan uses swapchain present modes for vsync; glfwSwapInterval requires an OpenGL context.
		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}
}

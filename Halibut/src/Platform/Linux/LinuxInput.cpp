#include "halibutpch.h"
#include "LinuxInput.h"

#include <GLFW/glfw3.h>
#include "Core/Application.h"
namespace HALIBUT
{
	UPtr<Input> Input::s_Instance = MakeUPtr<LinuxInput>();

	bool LinuxInput::IsKeyPressedImpl(int keycode)
	{
		auto window = static_cast<GLFWwindow*>((GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow());

		auto statu = glfwGetKey(window, keycode);
		return statu == GLFW_PRESS || statu == GLFW_REPEAT;
	}
	bool LinuxInput::IsMouseButtonPressedImpl(int button)
	{
		auto window = static_cast<GLFWwindow*>((GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow());
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}
	std::pair<float, float> LinuxInput::GetMousePositionImpl()
	{
		auto window = static_cast<GLFWwindow*>((GLFWwindow*)Application::Get()->GetWindow()->GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return { (float)xpos, (float)ypos };
	}
	float LinuxInput::GetMouseXImpl()
	{
		auto [x, y] = GetMousePositionImpl();
		return x;
	}
	float LinuxInput::GetMouseYImpl()
	{
		auto [x, y] = GetMousePositionImpl();
		return y;
	}
}

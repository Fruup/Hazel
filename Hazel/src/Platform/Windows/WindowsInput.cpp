#include "hzpch.h"
#include "Platform/Windows/WindowsInput.h"

#include "Hazel/Core/Application.h"
#include <GLFW/glfw3.h>

#include "Hazel/Events/GamepadEvent.h"
#include "Platform/Windows/Input/WindowsGamepad.h"

namespace Hazel {

	bool WindowsInput::IsKeyPressedImpl(KeyCode key)
	{
		auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool WindowsInput::IsMouseButtonPressedImpl(MouseCode button)
	{
		auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	std::pair<float, float> WindowsInput::GetMousePositionImpl()
	{
		auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	float WindowsInput::GetMouseXImpl()
	{
		auto[x, y] = GetMousePositionImpl();
		return x;
	}

	float WindowsInput::GetMouseYImpl()
	{
		auto[x, y] = GetMousePositionImpl();
		return y;
	}

	void WindowsInput::OnUpdateImpl(Timestep time)
	{
		// Poll state for every gamepad
		for (const auto& gamepad : m_Gamepads)
		{
			gamepad->PollState();
		}
	}

	Ref<Gamepad> WindowsInput::CreateGamepadImpl(int id, const char* name)
	{
		auto& pad = std::make_shared<WindowsGamepad>(id, name);
		m_Gamepads.push_back(pad);
		return pad;
	}

	void WindowsInput::InitImpl()
	{
		// Scan for existing gamepads
		for (int i = GLFW_JOYSTICK_1; i < GLFW_JOYSTICK_LAST; i++)
		{
			if (glfwJoystickIsGamepad(i))
			{
				// Add this gamepad
				const auto& _ = Input::CreateGamepad(i, glfwGetJoystickName(i));

				HZ_CORE_INFO("Found gamepad with id {0}", i);
			}
		}

		// Register callbacks
		glfwSetJoystickCallback([](int jid, int event)
			{
				if (glfwJoystickIsGamepad(jid))
				{
					switch (event)
					{
					case GLFW_CONNECTED:
					{
						// Add new gamepad
						const auto& pad = Input::CreateGamepad(jid, glfwGetGamepadName(jid));

						GamepadConnectedEvent e(jid);
						Application::Get().OnEvent(e);

						HZ_CORE_INFO("New gamepad with id {0} connected", jid);
						break;
					}
					case GLFW_DISCONNECTED:
					{
						// Find gamepad
						//const auto& pad = Input::FindGamepad(jid);

						GamepadDisconnectedEvent e(jid);
						Application::Get().OnEvent(e);

						HZ_CORE_INFO("Gamepad with id {0} disconnected", jid);
						break;
					}
					default:
						HZ_CORE_ERROR("Invalid joystick event ({0})", event);
					}
				}
			});
	}

	void WindowsInput::ShutdownImpl()
	{
		// Delete gamepads
#if defined HZ_DEBUG
		for (const auto& pad : m_Gamepads)
			HZ_ASSERT(pad.use_count() == 1, "Gamepad still in use somewhere");
#endif

		m_Gamepads.clear();
	}
}

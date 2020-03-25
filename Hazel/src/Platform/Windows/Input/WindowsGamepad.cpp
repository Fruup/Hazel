#include "hzpch.h"
#include "WindowsGamepad.h"

#include "GLFW/glfw3.h"

namespace Hazel
{
	bool WindowsGamepad::PollStateImpl()
	{
		return glfwGetGamepadState(m_Id, (GLFWgamepadstate*)&m_State);
	}
}

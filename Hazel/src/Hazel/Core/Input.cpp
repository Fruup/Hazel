#include "hzpch.h"
#include "Hazel/Core/Input.h"

#ifdef HZ_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsInput.h"
#endif

namespace Hazel {

	Scope<Input> Input::s_Instance = Input::Create();

	void Input::OnUpdate()
	{
		for (const auto& pad : s_Instance->m_Gamepads)
			pad->PollState();
	}

	Ref<Gamepad> Input::FindGamepad(int id)
	{
		for (const auto& pad : s_Instance->m_Gamepads)
			if (pad->GetId() == id)
				return pad;

		return Ref<Gamepad>();
	}

	Scope<Input> Input::Create()
	{
	#ifdef HZ_PLATFORM_WINDOWS
		return CreateScope<WindowsInput>();
	#else
		HZ_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
	#endif
	}
}

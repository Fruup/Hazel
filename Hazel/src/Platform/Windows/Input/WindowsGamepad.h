#pragma once

#include "Hazel/Input/Gamepad.h"

namespace Hazel
{
	class WindowsGamepad : public Gamepad
	{
	public:
		WindowsGamepad(int id, const char* name) : Gamepad(id, name) {}

		virtual bool PollStateImpl() override;
	};
}

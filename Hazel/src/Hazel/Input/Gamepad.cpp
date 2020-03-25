#include "hzpch.h"
#include "Gamepad.h"

#include "Hazel/Core/Application.h"
#include "Hazel/Events/GamepadEvent.h"

namespace Hazel
{
	Gamepad::Gamepad(int id, const char* name) :
		m_Id(id), m_Name(name)
	{
		// Reset data
		memset(&m_State, 0, sizeof(State));
		memset(&m_StateUncorrected, 0, sizeof(State));

		for (size_t i = 0; i < 4; i++)
			m_Config.deadzone[i] = .1f, m_Config.saturation[i] = .9f;
	}

	bool Gamepad::PollState()
	{
		if (PollStateImpl())
		{
			CorrectState();
			GenerateEvents();

			// Store state
			memcpy(&m_PrevState, &m_State, sizeof(State));

			return true;
		}

		return false;
	}

	void Gamepad::CorrectState()
	{
		// Copy state
		memcpy(&m_StateUncorrected, &m_State, sizeof(State));

		// Correct raw input data...

		// TODO: fix

		// Sticks
		/*for (int stick = 0; stick < 2; ++stick)
		{
			const float x = m_State.axes[stick * 2 + 0];
			const float y = m_State.axes[stick * 2 + 1];
			const float invDelta = 1.0f / (m_Config.saturation[stick] - m_Config.deadzone[stick]);

			float mag = sqrtf(x * x + y * y);
			const float a = mag == 0.0f ? 0.0f : acosf(x / mag);
			mag = fmaxf(fminf((mag - m_Config.deadzone[stick]) * invDelta, 1.0f), 0.0f);

			// Scale
			m_State.axes[stick * 2 + 0] = copysignf(cosf(a) * mag, x);
			m_State.axes[stick * 2 + 1] = copysignf(sinf(a) * mag, y);
		}*/

		// Triggers
		/*for (int trigger = 0; trigger < 2; ++trigger)
		{
			const float x = m_State.axes[trigger + 4];
			const float invDelta = 1.0f / (m_Config.saturation[trigger + 2] - m_Config.deadzone[trigger + 2]);

			// Scale
			m_State.axes[trigger + 4] = fmaxf(fminf((fabs(x) - m_Config.deadzone[trigger + 2]) * invDelta, 1.0f), 0.0f);
		}*/
	}

	void Gamepad::GenerateEvents()
	{
		for (int i = 0; i < NumButtons; i++)
		{
			if (m_PrevState.buttons[i] && !m_State.buttons[i])
				Application::PushEvent(GamepadButtonReleasedEvent(Input::FindGamepad(GetId()), (GamepadButton)i));
			else if (!m_PrevState.buttons[i] && m_State.buttons[i])
				Application::PushEvent(GamepadButtonPressedEvent(Input::FindGamepad(GetId()), (GamepadButton)i));
		}

		for (int i = 0; i < NumAxes; ++i)
			if (m_State.axes[i] != m_PrevState.axes[i])
				Application::PushEvent(GamepadAxisMovedEvent(Input::FindGamepad(GetId()), i));
	}
}

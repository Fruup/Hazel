#pragma once

#include "Hazel/Events/Event.h"

namespace Hazel
{
#define GAMEPAD_EVENT_CLASS(type) \
	EVENT_CLASS_TYPE(type) \
	EVENT_CLASS_CATEGORY(EventCategoryGamepad)

	class GamepadEvent : public Event
	{
	public:
		GamepadEvent() = delete;

		inline const Ref<Gamepad>& GetGamepad() const { return m_Gamepad; }
		inline int GetGamepadId() const { return m_Gamepad->GetId(); }
		inline const std::string& GetGamepadName() const { return m_Gamepad->GetName(); }

	protected:
		GamepadEvent(const Ref<Gamepad>& gamepad) :
			m_Gamepad(gamepad) {}

	private:
		Ref<Gamepad> m_Gamepad;
	};

	class GamepadConnectedEvent : public GamepadEvent
	{
	public:
		GamepadConnectedEvent(const Ref<Gamepad>& gamepad) :
			GamepadEvent(gamepad) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "GamepadConnectedEvent: " << GetGamepadName() << " (" << GetGamepadId() << ")";
			return ss.str();
		}

		GAMEPAD_EVENT_CLASS(GamepadConnected)
	};

	class GamepadDisconnectedEvent : public GamepadEvent
	{
	public:
		GamepadDisconnectedEvent(const Ref<Gamepad>& gamepad) :
			GamepadEvent(gamepad) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "GamepadDisconnectedEvent: " << GetGamepadName() << " (" << GetGamepadId() << ")";
			return ss.str();
		}

		GAMEPAD_EVENT_CLASS(GamepadDisconnected)
	};

	class GamepadButtonPressedEvent : public GamepadEvent
	{
	public:
		GamepadButtonPressedEvent(const Ref<Gamepad>& gamepad, GamepadButton button) :
			GamepadEvent(gamepad), m_Button(button) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "GamepadButtonPressedEvent: " << GetGamepadName() << " (" << GetGamepadId() << "), button " << (int)m_Button;
			return ss.str();
		}

		inline GamepadButton GetButton() const { return m_Button; }
		GAMEPAD_EVENT_CLASS(GamepadButtonPressed)

	private:
		GamepadButton m_Button;
	};

	class GamepadButtonReleasedEvent : public GamepadEvent
	{
	public:
		GamepadButtonReleasedEvent(const Ref<Gamepad>& gamepad, GamepadButton button) :
			GamepadEvent(gamepad), m_Button(button) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "GamepadButtonReleasedEvent: " << GetGamepadName() << " (" << GetGamepadId() << "), button " << (int)m_Button;
			return ss.str();
		}

		inline GamepadButton GetButton() const { return m_Button; }
		GAMEPAD_EVENT_CLASS(GamepadButtonReleased)

	private:
		GamepadButton m_Button;
	};

	class GamepadAxisMovedEvent : public GamepadEvent
	{
	public:
		GamepadAxisMovedEvent(const Ref<Gamepad>& gamepad, int axis) :
			GamepadEvent(gamepad), m_Axis(axis) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "GamepadAxisMovedEvent: " << GetGamepadName() << " (" << GetGamepadId()
				<< "), axis " << (int)m_Axis << ", " << GetGamepad()->GetState().axes[m_Axis];
			return ss.str();
		}

		inline int GetAxis() const { return m_Axis; }
		GAMEPAD_EVENT_CLASS(GamepadAxisMoved)

	private:
		int m_Axis;
	};
}
